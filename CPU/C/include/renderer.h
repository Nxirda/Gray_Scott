#pragma once

#include <SDL2/SDL.h>

#include "types.h"
#include "simulation.h"
#include "cli_handler.h"

typedef struct SDL_config_s
{
    u32 dim_x;
    u32 dim_y;
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
} SDL_config_t;

extern SDL_config_t render_init(args_t *args);
extern void render_cleanup(SDL_config_t *config);

extern void render_gray_scott(SDL_config_t config, chemicals_t const* chemicals);
