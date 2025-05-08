#include <stdio.h>

#include "simulation.h"
#include "cli_handler.h"
#include "renderer.h"

void tmp_print(chemicals_t *chem)
{
    real (*u)[chem->y_size] = make_2d_span(real, , chem->u, chem->y_size);

    for(u64 i = 0; i < chem->x_size; i++)
    {
        for(u64 j = 0; j < chem->y_size; j++)
        {
            printf("|%3.2f", u[i][j]);
        }
        printf("|\n");
    }
}

int main(int argc, char **argv)
{
    args_t args;
    parse_arguments(argc, argv, &args);

    chemicals_t uv_in;   
    chemicals_t uv_out; 
    
    FILE *fp = fopen(args.file_name, "wb");
    
    if(!args.interactive)  
    {
        uv_in = new_chemicals(args.num_rows, args.num_cols);
        uv_out = zeros_chemicals(args.num_rows, args.num_cols);
        for(u64 i = 0; i < args.steps; i++)
        {
            simulation_step(uv_in, uv_out);
            swap_chemicals(&uv_in, &uv_out);

            if(i % args.output_frequency == 0)
                write_data(fp, &uv_in);
        }
    }
    else
    { 
        SDL_config_t sdl_conf = render_init(&args, (u32)args.num_rows, (u32)args.num_cols);
        
        uv_in = new_chemicals(args.num_rows, args.num_cols);
        uv_out = zeros_chemicals(args.num_rows, args.num_cols);
        
        //tmp_print(&uv_in);
        for(u64 i = 0; i < args.steps; i++)
        {
            simulation_step(uv_in, uv_out);
            swap_chemicals(&uv_in, &uv_out);

            printf("=== STEP %lld === \n", i);
            if(i % args.output_frequency == 0)
                render_gray_scott(sdl_conf, uv_in);

        }
        render_cleanup(&sdl_conf);
    }

    free_chemicals(uv_in);
    free_chemicals(uv_out);

    fclose(fp);
    return 0;
}
