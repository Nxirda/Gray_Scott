#pragma once

#include "types.h"

#define DIFFUSION_RATE_U    REAL_TYPE(0.1)
#define DIFFUSION_RATE_V    REAL_TYPE(0.05)
#define FEEDRATE            REAL_TYPE(0.014)
#define KILLRATE            REAL_TYPE(0.054)
#define DELTA_T             REAL_TYPE(1.0)

#define STENCIL_ORDER       3
#define STENCIL_OFFSET      1

const real STENCIL_WEIGHTS[STENCIL_ORDER][STENCIL_ORDER] = 
{
    {0.25, 0.5, 0.25},
    { 0.5, 0.0, 0.5 },
    {0.25, 0.5, 0.25}
};

#define PADDING_OFFSET_X    1
#define PADDING_OFFSET_Y    1
