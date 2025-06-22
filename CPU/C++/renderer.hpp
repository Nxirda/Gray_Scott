#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <cstdint>
#include <stdexcept>

#include "colormap.h"
#include <kwk/kwk.hpp>

using real = float;  // or double, if preferred

class Renderer {
public:
    Renderer(std::size_t width, std::size_t height) : width(width), height(height)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error("SDL could not initialize!");
    
        int SDL_width   = static_cast<int>(width);
        int SDL_height  = static_cast<int>(height);

        window      = SDL_CreateWindow("Gray Scott", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SDL_width, SDL_height, SDL_WINDOW_SHOWN);
        renderer    = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture     = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING, SDL_width, SDL_height);
    }

    ~Renderer()
    {
        if (texture)    SDL_DestroyTexture(texture);
        if (renderer)   SDL_DestroyRenderer(renderer);
        if (window)     SDL_DestroyWindow(window);
        SDL_Quit();
    }

    template<kwk::concepts::container Container>
    void render(std::size_t d0, std::size_t d1, Container const& grid)
    {
        if (d0 != width || d1 != height)
            throw std::runtime_error("Grid size mismatch");

        std::vector<std::uint32_t> pixels(width * height);
        auto pixel_table = kwk::view{ kwk::source = pixels, kwk::of_size(d0, d1), kwk::with_strides(1, d0) };
        
        kwk::transform(kwk::cpu, [&](auto e)
        {
            std::uint32_t value = 0;

            if(e < 0.0f)
                value = 0;
             else if(e > 1.0f)
                 value = 1;
             else
                 value = static_cast<std::uint32_t>(e * 2.0f * 255.0f);

            return colormap[value];
        }, pixel_table, grid);

        SDL_UpdateTexture(texture, nullptr, pixels.data(), d0 * sizeof(std::uint32_t));
        SDL_Delay(5);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }


private:
    SDL_Window* window      = nullptr;
    SDL_Renderer* renderer  = nullptr;
    SDL_Texture* texture    = nullptr;
    std::size_t width;
    std::size_t height;
};

