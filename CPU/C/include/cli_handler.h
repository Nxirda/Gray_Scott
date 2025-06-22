#pragma once

#include "types.h"

typedef struct args_s
{
    u64 num_rows;
    u64 num_cols;
    u64 output_frequency;
    u64 steps;
    u8 interactive;
    char *file_name;
} args_t;

extern void parse_arguments(int argc, char *argv[argc+1], args_t *args);
