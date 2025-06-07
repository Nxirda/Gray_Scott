#include <stdio.h>

#include "simulation.h"
#include "cli_handler.h"
#include "renderer.h"
#include "logs.h"

// Where shall that be ??
void tmp_print(chemicals_t *chem)
{
    real (*u)[chem->y_size] = make_2D_span(real, , chem->u, chem->y_size);

    for(u64 i = 0; i < chem->x_size; i++)
    {
        for(u64 j = 0; j < chem->y_size; j++)
        {
            //printf("|%lld%lld", i, j);
            printf("|%3.2f", u[i][j]);
        }
        printf("|\n");
    }
}

void tmp_print_simd(chemicals_t *chem)
{
    real (*u)[chem->y_size][4] = make_3D_span(real, , chem->u, chem->y_size, 4);

    for(u64 i = 0; i < chem->x_size; i++)
    {
        for(u64 j = 0; j < chem->y_size; j++)
        {
            printf("[");
            for(u64 k = 0; k < 4; k++)
            {
                //printf("|%lld%lld", i, j);
                printf("%3.2f ", u[i][j][k]);
            }
            printf("] ");
        }
        printf("|\n");
    }
}

int main(int argc, char **argv)
{
    args_t args;
    parse_arguments(argc, argv, &args);
        
    // In debug print a logo and the args of the sim or do it with -v maybe

    chemicals_t uv_in;   
    chemicals_t uv_out; 
     
    if(!args.interactive)  
    {
        FILE *fp = fopen(args.file_name, "wb");
        if(!fp)
            gs_error_print("Couldn't open file : %s", args.file_name);
        
        uv_in   = new_chemicals(args.num_rows, args.num_cols);
        uv_out  = zeros_chemicals(args.num_rows, args.num_cols);
       
        gs_debug_print("Num rows : %lld; num cols : %lld", args.num_rows, args.num_cols);
        gs_debug_print("X size : %lld; Y size : %lld", uv_in.x_size, uv_in.y_size);
        
        for(u64 i = 0; i < args.steps; i++)
        {
            simulation_step(&uv_in, &uv_out);
            swap_chemicals(&uv_in, &uv_out);

            if(i % args.output_frequency == 0)
                write_data(fp, &uv_in);
        }

        fclose(fp);
    }
    else
    { 
        SDL_config_t sdl_conf = render_init(&args);
 
        uv_in   = new_chemicals(args.num_rows, args.num_cols);
        uv_out  = zeros_chemicals(args.num_rows, args.num_cols);
        
        chemicals_t tmp; 
        
        for(u64 i = 0; i < args.steps; i++)
        {
            simulation_step(&uv_in, &uv_out);
            swap_chemicals(&uv_in, &uv_out);

            if(i % args.output_frequency == 0)
            {
                tmp = to_scalar_layout(&uv_in);
                render_gray_scott(sdl_conf, &tmp);
            }

        }

        free_chemicals(&tmp);
        render_cleanup(&sdl_conf);
    }

    free_chemicals(&uv_in);
    free_chemicals(&uv_out);

    return 0;
}
