#pragma once

#include <stdio.h>

#include "types.h"

#define make_2D_span(type, attr, ptr, dim2)         (type (*attr)[dim2])        (ptr)
#define make_3D_span(type, attr, ptr, dim2, dim3)   (type (*attr)[dim2][dim3])  (ptr)

typedef struct chemicals_s
{    
    u64 x_size;
    u64 y_size;
    u64 nb_members; 
    real *restrict u;
    real *restrict v;  
} chemicals_t;

extern chemicals_t new_chemicals(u64 x, u64 y);
extern chemicals_t zeros_chemicals(u64 x, u64 y);
extern void free_chemicals(chemicals_t *chemical);

extern void simulation_step(chemicals_t const* in, chemicals_t* out);
extern void swap_chemicals(chemicals_t *ptr_1, chemicals_t *ptr_2);

extern void write_data(FILE *fp, chemicals_t const *chemical);
extern chemicals_t read_data(FILE *fp);

extern chemicals_t to_scalar_layout(chemicals_t const *in);
