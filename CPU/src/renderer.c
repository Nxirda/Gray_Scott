#include "renderer.h"
#include "colormap.h"

#define MIN_SIZE 400

SDL_config_t render_init(args_t *args, u32 dim_x, u32 dim_y)
{
    SDL_config_t config;

    config.dim_x = dim_x;
    config.dim_y = dim_y;
    
    if((dim_x <= MIN_SIZE) || (dim_x > 640) 
     || (dim_y <= MIN_SIZE) || (dim_y > 480))
    {
        printf("Dimensions are to small for visualization reset setting to 16x16\n");
    
        config.dim_x    = MIN_SIZE;
        config.dim_y    = MIN_SIZE;
        args->num_cols  = MIN_SIZE;
        args->num_rows  = MIN_SIZE;
    }
    config.window = SDL_CreateWindow("Gray Scott Simulation", SDL_WINDOWPOS_UNDEFINED
             , SDL_WINDOWPOS_UNDEFINED, (i32)config.dim_x, (i32)config.dim_y, SDL_WINDOW_SHOWN);

     config.renderer = SDL_CreateRenderer(config.window, -1
             , SDL_RENDERER_ACCELERATED);

     config.texture = SDL_CreateTexture(config.renderer
             , SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING
             , (i32)config.dim_x, (i32)config.dim_y);

     return config;
}

void render_cleanup(SDL_config_t *config)
{
     SDL_DestroyTexture(config->texture);
     SDL_DestroyRenderer(config->renderer);
     SDL_DestroyWindow(config->window);
     SDL_Quit();
}


void render_gray_scott(SDL_config_t config, chemicals_t chemical)
{
    real(*v_map)[chemical.y_size] = make_2d_span(real, ,chemical.v, chemical.y_size);

    u32 *pixels = (Uint32 *)malloc((u64)config.dim_x * (u64)config.dim_y * sizeof(Uint32));

    for(u32 j = 0; j < config.dim_y; j++)
    {
        for(u32 i = 0; i < config.dim_x; i++)
        {
            // V is supposed to be in the [0,1] range
            u32 value = (u32)(v_map[i][j] * 255.0f); //pixel_map[i][j];
            Uint32 color = colormap[value];

            pixels[j * config.dim_x + i] = color;
        }
    }

    SDL_UpdateTexture(config.texture, NULL, pixels, (i32)(config.dim_x * sizeof(*pixels)));
    SDL_Delay(10);
    SDL_RenderClear(config.renderer);
    SDL_RenderCopy(config.renderer, config.texture, NULL, NULL);
    SDL_RenderPresent(config.renderer);
    free(pixels);
}
