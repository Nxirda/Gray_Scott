#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <numeric>
#include <chrono>
#include <algorithm>

#include <kwk/kwk.hpp>
#include "tile.hpp"
#include <eve/eve.hpp>

#include "renderer.hpp"

using real = float;
using wide_t = eve::wide<real>; 
using namespace kwk::literals;

constexpr auto context = kwk::cpu;

constexpr real KILL_RATE        { 0.054f };
constexpr real FEED_RATE        { 0.014f };
constexpr real DT               { 1.0f   };
constexpr real DIFFUSION_RATE_U { 0.1f   };
constexpr real DIFFUSION_RATE_V { 0.05f  };

// Representing the offset from the sides of the simulation, 
// boundary conditions are defined as 0 everywhere   
constexpr std::size_t PADDING = 1;

/*
    | 0.25 | 0.5 | 0.25 |
    |  0.5 | 0.0 | 0.5  |
    | 0.25 | 0.5 | 0.25 |
*/
constexpr real STENCIL_WEIGHTS[]    = { 0.25f, 0.5f, 0.25f, 0.5f, 0.0f, 0.5f, 0.25f, 0.5f, 0.25f };
constexpr auto stencil_shape        = kwk::of_size(3_c, 3_c);
// Due to tiling we need a 4D view on the weights, that s how the indexing works in the algos
constexpr auto weights = kwk::view{ kwk::source = STENCIL_WEIGHTS, kwk::of_size(1_c, 1_c, 3_c, 3_c) };


template<kwk::concepts::container Container>
void init_chemicals(Container &u, Container &v, std::size_t d0, std::size_t d1)
{
    const std::size_t d0_region = d0 - 2 * PADDING;
    const std::size_t d1_region = d1 - 2 * PADDING;

    const std::size_t d0_start  = ((7 * d0_region) / 16) - 4;
    const std::size_t d0_end    = ((8 * d0_region) / 16) - 4;

    const std::size_t d1_start  = (7 * d1_region) / 16;
    const std::size_t d1_end    = (8 * d1_region) / 16;

    const auto region_stride    = kwk::with_strides(d1, 1);
    const auto region_size      = kwk::of_size(d0_region, d1_region);

    auto pattern = [=](auto i0, auto i1)
    { 
        return static_cast<real>((i0 >= d0_start) && (i0 < d0_end) 
                              && (i1 >= d1_start) && (i1 < d1_end));
    };

    auto region_u = kwk::view{kwk::source = &u(PADDING, PADDING), region_stride, region_size};
    auto region_v = kwk::view{kwk::source = &v(PADDING, PADDING), region_stride, region_size};

    kwk::for_each_index(context, [&](auto &elt, auto i0, auto i1)
    {
        elt = 1.f - pattern(i0, i1);
    }, region_u);
    
    kwk::for_each_index(context, [&](auto &elt, auto i0, auto i1)
    {
        elt = pattern(i0, i1);
    }, region_v);
}

template<kwk::concepts::container Container>
void process_kwk(Container const& iu, Container const& iv,
                 Container      & ou, Container      & ov,
                 std::size_t d0, std::size_t d1)
{
    const auto region_stride    = kwk::with_strides(d1, 1); 
    const auto region_shape     = kwk::of_size(d0 - 2 * PADDING, d1 - 2 * PADDING);

    const auto offset   = kumi::tuple{1_c, 1_c};
    const auto tiled_u  = paving_tiles(iu, stencil_shape, offset);
    const auto tiled_v  = paving_tiles(iv, stencil_shape, offset); 

    // View for the output
    auto ou_view = kwk::view{ kwk::source = &ou(PADDING,PADDING), region_shape, region_stride };
    auto ov_view = kwk::view{ kwk::source = &ov(PADDING,PADDING), region_shape, region_stride };
   
    kwk::for_each([&]( real& out_u, real& out_v, auto const& tile_u, auto const& tile_v )
    {
        const auto u = tile_u(0, 0, 1, 1);
        const auto v = tile_v(0, 0, 1, 1);
        const auto uvv = u * v * v;

        auto full_u = 0.f;
        auto full_v = 0.f;

        kwk::for_each([&](real const& uu, real const& vv, real const& weight)
        {
            full_u += weight * (uu - u);
            full_v += weight * (vv - v);

        }, tile_u, tile_v, weights); 

        auto du = DIFFUSION_RATE_U * full_u - uvv + FEED_RATE * (1.0f - u);
        auto dv = DIFFUSION_RATE_V * full_v + uvv - (FEED_RATE + KILL_RATE) * v;

        out_u = u + du * DT;
        out_v = v + dv * DT;             
    }, ou_view, ov_view, tiled_u, tiled_v);
}

template<kwk::concepts::container Container>
void process_kwk_simd(Container const& iu, Container const& iv,
                 Container      & ou, Container      & ov,
                 std::size_t d0, std::size_t d1)
{
    const auto region_stride    = kwk::with_strides(d1, wide_t::size()); 
    const auto region_shape     = kwk::of_size(d0 - 2 * PADDING, d1 - 2 * PADDING);

    const auto offset   = kumi::tuple{1_c, wide_t::size()};
    const auto tiled_u  = paving_tiles(iu, stencil_shape, offset);
    const auto tiled_v  = paving_tiles(iv, stencil_shape, offset); 

    // View for the output
    auto ou_view = kwk::view{ kwk::source = &ou(PADDING,PADDING), region_shape, region_stride };
    auto ov_view = kwk::view{ kwk::source = &ov(PADDING,PADDING), region_shape, region_stride };
   
    kwk::for_each([&]( real& out_u, real& out_v, auto const& tile_u, auto const& tile_v )
    {
        const wide_t u = eve::load( &tile_u(0,0,1,1) );
        const wide_t v = eve::load( &tile_v(0,0,1,1) );
        const wide_t uvv = u * v * v;

        wide_t full_u{ 0.f };
        wide_t full_v{ 0.f };

        kwk::for_each([&](real const& uu, real const& vv, real const& weight)
        {
            const wide_t uu_vector = eve::load(&uu);
            const wide_t vv_vector = eve::load(&vv);

            full_u += weight * (uu_vector - u);
            full_v += weight * (vv_vector - v);

        }, tile_u, tile_v, weights); 

        auto du = DIFFUSION_RATE_U * full_u - uvv + FEED_RATE * (1.0f - u);
        auto dv = DIFFUSION_RATE_V * full_v + uvv - (FEED_RATE + KILL_RATE) * v;

        du = u + du * DT;
        dv = v + dv * DT;             
       
        eve::store(du, &out_u);
        eve::store(dv, &out_v);

    }, ou_view, ov_view, tiled_u, tiled_v);
}

//
int main(int argc, char *argv[])
{

    if(argc != 5)
    {
        std::cerr << "Usage is " << argv[0] << " <rows> <columns> <images> <interactive> \n";
        exit(1);
    }

    std::size_t d0    { std::stoul(argv[1]) + 2 * PADDING };
    std::size_t d1    { std::stoul(argv[2]) + 2 * PADDING };
    std::size_t steps { std::stoul(argv[3]) };
    std::size_t inter { std::stoul(argv[4]) };

    // Temporary work images
    std::vector<real> u1(d0*d1 , 0.f);
    std::vector<real> v1(d0*d1 , 0.f);
    std::vector<real> u2(d0*d1 , 0.f);
    std::vector<real> v2(d0*d1 , 0.f);

    const auto shape = kwk::of_size(d0, d1);

    auto u1_kwk = kwk::table{ kwk::source = u1, shape };
    auto v1_kwk = kwk::table{ kwk::source = v1, shape };
    auto u2_kwk = kwk::table{ kwk::source = u2, shape };
    auto v2_kwk = kwk::table{ kwk::source = v2, shape };

    init_chemicals(u1_kwk, v1_kwk, d0, d1);

    if (!inter)
    {
        // Iterations;
        for (std::size_t step = 0; step < steps; ++step)
        { 
            process_kwk( u1_kwk, v1_kwk, u2_kwk, v2_kwk, d0, d1 );

            u1_kwk.swap(u2_kwk);
            v1_kwk.swap(v2_kwk); 
        }

        // Final product
        real product = kwk::inner_product(u1_kwk, v1_kwk, 0.f);
        std::cout << "Inner product : " << product << std::endl; 
    }
    else
    {
        Renderer renderer(d0, d1);

        init_chemicals(u1_kwk, v1_kwk, d0, d1);

        // Iterations;
        for (std::size_t step = 0; step < steps; ++step)
        { 
            process_kwk_simd( u1_kwk, v1_kwk, u2_kwk, v2_kwk, d0, d1 );

            u1_kwk.swap(u2_kwk);
            v1_kwk.swap(v2_kwk);

            renderer.render(d0, d1, v1_kwk);
        }
    }
    return 0;
}
