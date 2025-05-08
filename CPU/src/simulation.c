#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "simulation.h"

// Error handling via asserts to be added

static const u64 ALIGNMENT = sizeof(real) * 8;

chemicals_t new_chemicals(u64 x, u64 y) 
{
    chemicals_t uv;
    uv.x_size = x;
    uv.y_size = y;
    uv.nb_members = 2;

    u64 size = (x * y);
    u64 bytes_size = uv.nb_members * (size * sizeof(real));

    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size);

    //Legal with restrict as per 
    // https://en.cppreference.com/w/c/language/restrict
    uv.u = (data);
    uv.v = (data + size);

    u64 x_start  = (7 * x) / 16;
    u64 x_end    = (8 * x) / 16;
    u64 y_start  = (7 * y) / 16;
    u64 y_end    = (8 * y) / 16;

    for(u64 i = 0; i < x; i++)
    {
        for(u64 j = 0; j < y; j++)
        {
            u64 idx = i * y + j;
            int in_region = (i >= x_start && i < x_end && 
                             j >= y_start && j < y_end);

            real pattern = (real)in_region;
            
            uv.u[idx] = REAL_TYPE(1.0) -  pattern;
            uv.v[idx] = pattern;
        }
    }
    return uv;
}

chemicals_t zeros_chemicals(u64 x, u64 y)
{
    chemicals_t uv;
    uv.x_size = x;
    uv.y_size = y;
    uv.nb_members = 2;

    u64 size = (x * y);
    u64 bytes_size = uv.nb_members * (size * sizeof(real));

    real *data = (real *)aligned_alloc(ALIGNMENT, bytes_size);
    if(!data)
    {
        fprintf(stderr, "[Error] Could not allocate mesh\n");
        exit(EXIT_FAILURE);
    }
    memset(data, 0, bytes_size); 

    uv.u = (data);
    uv.v = (data + size);

    return uv;
}

void swap_chemicals(chemicals_t *chem_1, chemicals_t *chem_2)
{
    chemicals_t tmp = *chem_1;
    *chem_1 = *chem_2;
    *chem_2 = tmp;
}

void simulation_step(const chemicals_t chem_in, chemicals_t chem_out)
{
    real u                  = REAL_TYPE(0.0); 
    real v                  = REAL_TYPE(0.0);
    real du                 = REAL_TYPE(0.0);
    real dv                 = REAL_TYPE(0.0);
    real full_u             = REAL_TYPE(0.0);
    real full_v             = REAL_TYPE(0.0);
    real weight             = REAL_TYPE(0.0);
    real sq_uv              = REAL_TYPE(0.0);
    
    u64 idx                 = 0;
    u64 stencil_start       = 0;
    u64 inner_idx           = 0;

    for(u64 i = STENCIL_OFFSET; i < chem_in.x_size - STENCIL_OFFSET; i++)
    {
        for(u64 j = STENCIL_OFFSET; j < chem_in.y_size - STENCIL_OFFSET; j++)
        {
            idx = i * chem_in.y_size + j;

            u = chem_in.u[idx];
            v = chem_in.v[idx];
            sq_uv = u * v * v;

            full_u = REAL_TYPE(0.0);
            full_v = REAL_TYPE(0.0);

            for(u64 k = 0; k < STENCIL_ORDER; k++)
            {
                stencil_start = (i + k - STENCIL_OFFSET) * chem_in.y_size - STENCIL_OFFSET;

                for(u64 l = 0; l < STENCIL_ORDER; l++)
                {
                    inner_idx = stencil_start + j + l;
                    weight = STENCIL_WEIGHTS[k][l];
                    full_u += weight * (chem_in.u[inner_idx] - u);
                    full_v += weight * (chem_in.v[inner_idx] - v);
                }
            }

            du = DIFFUSION_RATE_U * full_u - sq_uv + FEEDRATE * (REAL_TYPE(1.0) - u);
            dv = DIFFUSION_RATE_V * full_v + sq_uv - (FEEDRATE + KILLRATE) * v;
            
            chem_out.u[idx] = u + du * DELTA_T;
            chem_out.v[idx] = v + dv * DELTA_T;
        }
    }
}

void free_chemicals(chemicals_t chemical)
{
    if(chemical.u)
        free(chemical.u);
}

void write_data(FILE *fp, chemicals_t *chem)
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
