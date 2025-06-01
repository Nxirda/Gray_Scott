#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>

#include <omp.h>

#include "constants.h"
#include "simulation.h"
#include "logs.h"

// Replace 256 by computed SIMD_VECTOR_LEN of the arch
#define SIMD_LEN        (256ULL/8ULL)
#define SIMD_WIDTH      (SIMD_LEN/sizeof(real))//4
#define SIMD_OFFSET_X   1ULL
#define SIMD_OFFSET_Y   1ULL

#define ALIGNMENT       64ULL
#define BLOCK_SIZE_X    64ULL
#define BLOCK_SIZE_Y    64ULL
                
#define aligned_3D_span(base, field, dim2)                                      \
    __builtin_assume_aligned(                                                   \
        make_3D_span(real, restrict, (base)->field, (base)->dim2, SIMD_WIDTH)   \
        , ALIGNMENT                                                             \
    );
 
static inline void update_top_bottom(chemicals_t *uv)
{ 
    real (*restrict u_span)[uv->y_size][SIMD_WIDTH]
        = aligned_3D_span(uv, u, y_size);

    real (*restrict v_span)[uv->y_size][SIMD_WIDTH] 
        = aligned_3D_span(uv, v, y_size);
   
    #pragma omp parallel for schedule(dynamic)
    for(u64 j = SIMD_OFFSET_Y; j < uv->y_size - SIMD_OFFSET_Y; j++)
    {
        for(u64 k = 0; k < SIMD_WIDTH-1; k++)
        {
            u_span[0][j][k + 1] = u_span[uv->x_size - 2][j][k];
            u_span[uv->x_size - 1][j][k] = u_span[1][j][k + 1];

        }

        for(u64 k = 0; k < SIMD_WIDTH-1; k++)
        {
            v_span[0][j][k + 1] = v_span[uv->x_size - 2][j][k];
            v_span[uv->x_size - 1][j][k] = v_span[1][j][k + 1];
        }
    }
}

chemicals_t new_chemicals(u64 x, u64 y) 
{
    assert(x % SIMD_WIDTH == 0);
    assert(y % SIMD_WIDTH == 0);
 
    chemicals_t uv;
    uv.nb_members = 2;
 
    u64 num_center_rows = x / SIMD_WIDTH;  
    u64 simd_x_size     = num_center_rows + 2;
    u64 simd_y_size     = (y + 2); // * SIMD_WIDTH;

    uv.x_size = simd_x_size;
    uv.y_size = simd_y_size; 

    u64 size = (uv.x_size * (uv.y_size * SIMD_WIDTH));
    u64 bytes_size = uv.nb_members * (size * sizeof(real));
    
    assert(bytes_size % ALIGNMENT == 0);
    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size);
    if(!data)
    {
        gs_error_print("Could not allocate %lld bytes for the mesh", bytes_size);
    }
    memset(data, 0, bytes_size);

    //Legal with restrict as per 
    // https://en.cppreference.com/w/c/language/restrict
    uv.u = (data);
    uv.v = (data + size);

    u64 x_start  = (7 * x) / 16;
    u64 x_end    = (8 * x) / 16;
    u64 y_start  = (7 * y) / 16;
    u64 y_end    = (8 * y) / 16;

    // Need to check but it s normally legal to do so 
    real (*restrict u_span)[uv.y_size][SIMD_WIDTH] 
        = aligned_3D_span(&uv, u, y_size);

    real (*restrict v_span)[uv.y_size][SIMD_WIDTH] 
        = aligned_3D_span(&uv, v, y_size);

    //#pragma omp parallel for 
    for(u64 simd_i = SIMD_OFFSET_X; simd_i < simd_x_size - SIMD_OFFSET_X; simd_i++)
    {
        for(u64 simd_j = SIMD_OFFSET_Y; simd_j < simd_y_size - SIMD_OFFSET_Y; simd_j++)
        {   
            for(u64 k = 0; k < SIMD_WIDTH; k++)
            {   
                u64 scalar_row = simd_i - 1 + k * num_center_rows;
                u64 scalar_col = simd_j - 1;

                real pattern = (real)( scalar_row >= x_start 
                                    && scalar_row <  x_end 
                                    && scalar_col >= y_start 
                                    && scalar_col <  y_end);
                
                u_span[simd_i][simd_j][k] = REAL_TYPE(1.0) - pattern;
                v_span[simd_i][simd_j][k] = pattern;
            }
        }
    }

    update_top_bottom(&uv);
    return uv;
}

chemicals_t zeros_chemicals(u64 x, u64 y)
{
    assert(x % SIMD_WIDTH == 0);
    assert(y % SIMD_WIDTH == 0);

    chemicals_t uv;
    uv.nb_members = 2;

    uv.x_size = (x/SIMD_WIDTH) + 2 ;
    uv.y_size = (y + 2); 

    u64 size = (uv.x_size * (uv.y_size * SIMD_WIDTH));
    u64 bytes_size = uv.nb_members * (size * sizeof(real));
    
    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size);
    if(!data)
    {
        gs_error_print("Could not allocate %lld bytes for the mesh", bytes_size);
    }
    memset(data, 0, bytes_size); 

    uv.u = (data);
    uv.v = (data + size);
    
    return uv;
}

chemicals_t to_scalar_layout(chemicals_t const *chem_in) 
{
    chemicals_t uv;
    uv.nb_members = chem_in->nb_members;
   
    u64 num_center_rows = chem_in->x_size - 2;
    u64 scalar_x_size   = num_center_rows * SIMD_WIDTH; 
    u64 scalar_y_size   = chem_in->y_size - 2; 

    uv.x_size = scalar_x_size;            
    uv.y_size = scalar_y_size;

    u64 size = (uv.x_size * uv.y_size);
    u64 bytes_size = uv.nb_members * (size * sizeof(real));

    // We might want to have a scalar_alignment
    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size);
    if(!data)
    {
        gs_error_print("Could not allocate %lld bytes for the mesh", bytes_size);
    }

    uv.u = (data);
    uv.v = (data + size);

    real (*restrict u_out)[uv.y_size]
        = make_2D_span(real, restrict, uv.u, uv.y_size);

    real (*restrict v_out)[uv.y_size] 
        = make_2D_span(real, restrict, uv.v, uv.y_size);

    const real (*restrict u_in)[chem_in->y_size][SIMD_WIDTH] 
        = aligned_3D_span(chem_in, u, y_size);
    
    const real (*restrict v_in)[chem_in->y_size][SIMD_WIDTH]
        = aligned_3D_span(chem_in, v, y_size);

    //#pragma omp parallel for
    for(u64 simd_i = SIMD_OFFSET_X; simd_i < chem_in->x_size - SIMD_OFFSET_X; simd_i++)
    {
        for(u64 simd_j = SIMD_OFFSET_Y; simd_j < chem_in->y_size - SIMD_OFFSET_Y; simd_j++)
        {   
            for(u64 k = 0; k < SIMD_WIDTH; k++)
            {   
                u64 scalar_row = simd_i - 1 + k * num_center_rows;
                u64 scalar_col = simd_j - 1;

                u_out[scalar_row][scalar_col] = u_in[simd_i][simd_j][k];
                v_out[scalar_row][scalar_col] = v_in[simd_i][simd_j][k]; 
            }
        }
    }
    return uv;
}
                
// This is the stencil operation 
#define STENCIL_OPERATION()                                                     \
do {                                                                            \
        const real u = u_span[i][j][k];                                         \
        const real v = v_span[i][j][k];                                         \
        const real sq_uv = u * v * v;                                           \
                                                                                \
        real full_u1 = STENCIL_WEIGHTS[0][0] * (u_span[i-1][j-1][k] - u);       \
        real full_v1 = STENCIL_WEIGHTS[0][0] * (v_span[i-1][j-1][k] - v);       \
        real full_u2 = STENCIL_WEIGHTS[0][1] * (u_span[i-1][j  ][k] - u);       \
        real full_v2 = STENCIL_WEIGHTS[0][1] * (v_span[i-1][j  ][k] - v);       \
        real full_u3 = STENCIL_WEIGHTS[0][2] * (u_span[i-1][j+1][k] - u);       \
        real full_v3 = STENCIL_WEIGHTS[0][2] * (v_span[i-1][j+1][k] - v);       \
        real full_u4 = STENCIL_WEIGHTS[1][0] * (u_span[i  ][j-1][k] - u);       \
        real full_v4 = STENCIL_WEIGHTS[1][0] * (v_span[i  ][j-1][k] - v);       \
        assert(STENCIL_WEIGHTS[1][1] == 0.0);                                   \
        full_u1 += STENCIL_WEIGHTS[1][2] * (u_span[i  ][j+1][k] - u);           \
        full_v1 += STENCIL_WEIGHTS[1][2] * (v_span[i  ][j+1][k] - v);           \
        full_u2 += STENCIL_WEIGHTS[2][0] * (u_span[i+1][j-1][k] - u);           \
        full_v2 += STENCIL_WEIGHTS[2][0] * (v_span[i+1][j-1][k] - v);           \
        full_u3 += STENCIL_WEIGHTS[2][1] * (u_span[i+1][j  ][k] - u);           \
        full_v3 += STENCIL_WEIGHTS[2][1] * (v_span[i+1][j  ][k] - v);           \
        full_u4 += STENCIL_WEIGHTS[2][2] * (u_span[i+1][j+1][k] - u);           \
        full_v4 += STENCIL_WEIGHTS[2][2] * (v_span[i+1][j+1][k] - v);           \
                                                                                \
        const real full_u = (full_u1 + full_u2) + (full_u3 + full_u4);          \
        const real full_v = (full_v1 + full_v2) + (full_v3 + full_v4);          \
                                                                                \
        const real du = (DIFFUSION_RATE_U * full_u - sq_uv)                     \
                        + (FEEDRATE * (REAL_TYPE(1.0) - u));                    \
        const real dv = (DIFFUSION_RATE_V * full_v + sq_uv)                     \
                        - ((FEEDRATE + KILLRATE) * v);                          \
                                                                                \
        u_span_out[i][j][k] = u + du * DELTA_T;                                 \
        v_span_out[i][j][k] = v + dv * DELTA_T;                                 \
} while(0)

void simulation_step(chemicals_t const* chem_in, chemicals_t* chem_out)
{
    assert(chem_in->u && chem_out->u);
    assert(chem_in->v && chem_out->v);
    assert(chem_in->x_size == chem_out->x_size);
    assert(chem_in->y_size == chem_out->y_size);
   
    const real (*restrict u_span)[chem_in->y_size][SIMD_WIDTH] 
        = aligned_3D_span(chem_in, u, y_size);

    const real (*restrict v_span)[chem_in->y_size][SIMD_WIDTH] 
        = aligned_3D_span(chem_in, v, y_size);

    real (*restrict u_span_out)[chem_out->y_size][SIMD_WIDTH] 
        = aligned_3D_span(chem_out, u, y_size);

    real (*restrict v_span_out)[chem_out->y_size][SIMD_WIDTH] 
        = aligned_3D_span(chem_out, v, y_size);
  
    const u64 nb_x      = (chem_in->x_size - 2 * SIMD_OFFSET_X) / BLOCK_SIZE_X; 
    const u64 last_j    = chem_in->y_size - SIMD_OFFSET_Y;

    const u64 last_bi   = SIMD_OFFSET_X + nb_x * BLOCK_SIZE_X;
    const u64 last_i    = chem_in->x_size - SIMD_OFFSET_X;
    
    #pragma omp parallel
    {
        #pragma omp for nowait schedule(dynamic)
        for(u64 bi = 0; bi < nb_x; ++bi)
        {
            const u64 i0 = SIMD_OFFSET_X + bi * BLOCK_SIZE_X;
            const u64 i1 = i0 + BLOCK_SIZE_X;

            for(u64 i = i0; i < i1; ++i)
            {
                for(u64 j = SIMD_OFFSET_Y; j < last_j; ++j)
                {
                    for(u64 k = 0; k < SIMD_WIDTH; ++k)
                    {
                        STENCIL_OPERATION();
                    }
                }
            }
        }
        
        #pragma omp for nowait schedule(dynamic)
        for(u64 i = last_bi; i < last_i; ++i)
        {
            for(u64 j = SIMD_OFFSET_Y; j < last_j; ++j)
            {
                for(u64 k = 0; k < SIMD_WIDTH; ++k)
                {
                    STENCIL_OPERATION();
                }
            }
        }
    }
    update_top_bottom(chem_out);
}

void swap_chemicals(chemicals_t *chem_1, chemicals_t *chem_2)
{
    assert(chem_1 && chem_2);

    chemicals_t tmp = *chem_1;
    *chem_1 = *chem_2;
    *chem_2 = tmp;
}

void free_chemicals(chemicals_t *chemical)
{
    if(chemical->u ) free(chemical->u);
}

void write_data(FILE *fp, chemicals_t const *chem)
{
    fwrite(&chem->x_size    , sizeof(chem->x_size)      , 1, fp);
    fwrite(&chem->y_size    , sizeof(chem->y_size)      , 1, fp);
    fwrite(&chem->nb_members, sizeof(chem->nb_members)  , 1, fp);

    // Works as u is ptr to the begening of a long serie of aligned data
    fwrite(chem->u, sizeof(*chem->u), chem->nb_members * chem->x_size * chem->y_size, fp);
}

chemicals_t read_data(FILE *fp)
{
    chemicals_t out;

    fread(&out.x_size, sizeof(out.x_size)           , 1, fp);
    fread(&out.y_size, sizeof(out.y_size)           , 1, fp);
    fread(&out.nb_members, sizeof(out.nb_members)   , 1, fp); 
  
    u64 size = out.x_size * out.y_size;
    u64 bytes_size = out.nb_members * size;
    
    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size); 
    fread(data, sizeof(*data), bytes_size, fp);

    out.u = (data);
    out.v = (data + size);

    return out;
}
