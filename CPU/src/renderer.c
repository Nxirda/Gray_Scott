#include "renderer.h"
#include "colormap.h"
#include "logs.h"

#define MIN_SIZE_X  640
#define MIN_SIZE_Y  360
#define MAX_SIZE_X  1920
#define MAX_SIZE_Y  1080

SDL_config_t render_init(args_t *args)
{
    SDL_config_t config;

    config.dim_x = (u32)args->num_rows;
    config.dim_y = (u32)args->num_cols;
     
    if(config.dim_x < MIN_SIZE_X)
    {
        gs_warn_print("Width dimensions is to small for visualization : %d", config.dim_x);
        gs_info_print("Width has been set to the min size : %d", MIN_SIZE_X);
    
        config.dim_x    = MIN_SIZE_X;
        args->num_rows  = MIN_SIZE_X;
    }
    else if(config.dim_x > MAX_SIZE_X)
    {
        gs_warn_print("Width dimension is to big for visualization : %d", config.dim_x);
        gs_info_print("Width has been set to the max size : %d", MAX_SIZE_X);

        config.dim_x    = MAX_SIZE_X;
        args->num_rows  = MAX_SIZE_X;
    }
    
    if(config.dim_y < MIN_SIZE_Y)
    {
        gs_warn_print("Height dimension is to small for visualization : %d", config.dim_y);
        gs_info_print("Height has been set to the min size : %d", MIN_SIZE_Y);
    
        config.dim_y    = MIN_SIZE_Y;
        args->num_cols  = MIN_SIZE_Y;
    }
    else if(config.dim_y > MAX_SIZE_Y)
    {
        gs_warn_print("Height dimension is to big for visualization : %d", config.dim_y);
        gs_info_print("Height has been set to the max size : %d", MAX_SIZE_Y);
    
        config.dim_y    = MAX_SIZE_Y;
        args->num_cols  = MAX_SIZE_Y;
    }

    config.window = SDL_CreateWindow("Gray Scott Simulation"
                , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
                , (i32)config.dim_x, (i32)config.dim_y, SDL_WINDOW_SHOWN);

     config.renderer = SDL_CreateRenderer(config.window, -1
                , SDL_RENDERER_ACCELERATED);

     config.texture = SDL_CreateTexture(config.renderer
                , SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING
                , (i32)config.dim_x, (i32)config.dim_y);


    gs_debug_print("Window size : %d %d", config.dim_x, config.dim_y);

     return config;
}

void render_cleanup(SDL_config_t *config)
{
     SDL_DestroyTexture(config->texture);
     SDL_DestroyRenderer(config->renderer);
     SDL_DestroyWindow(config->window);
     SDL_Quit();
}

// There is some padding to handle there
void render_gray_scott(SDL_config_t config, chemicals_t chemical)
{
    const real(* restrict v_map)[chemical.y_size] = 
        make_2D_span(real, restrict, chemical.v, chemical.y_size);
    
    u64 nb_bytes    = (config.dim_x * config.dim_y) * sizeof(u32);
    u32 *pixels     = (u32*)malloc(nb_bytes);
    if(!pixels)
    {
        gs_error_print("Couldnt allocate %lld bytes from memory for the rendering"
                , nb_bytes);
    }

    u32 value = REAL_TYPE(0.0);
    // Needs correction for the bounds of the render
    for(u32 j = 0; j < config.dim_y; j++)
    {
        for(u32 i = 0; i < config.dim_x; i++)
        {
            // Force the Clamping of the value between 0 and 1 
            // and mul it by 2 to use the full palette
            if(v_map[i][j] < REAL_TYPE(0.0))
                value = REAL_TYPE(0.0);
            else if(v_map[i][j] > REAL_TYPE(1.0))
                value = REAL_TYPE(1.0);
            else
                value = (u32)(v_map[i][j] * REAL_TYPE(2.0) * REAL_TYPE(255.0));

            u32 color = colormap[value];
            pixels[j * config.dim_x + i] = color;
        }
    }

    SDL_UpdateTexture(config.texture, NULL, pixels, (i32)(config.dim_x * sizeof(*pixels)));
    SDL_Delay(5);
    SDL_RenderClear(config.renderer);
    SDL_RenderCopy(config.renderer, config.texture, NULL, NULL);
    SDL_RenderPresent(config.renderer);
    free(pixels);
}
