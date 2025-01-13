#ifndef RENDERING_H
#define RENDERING_H

#include <SDL2/SDL.h>

void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha);
SDL_Texture *load_image(SDL_Renderer *renderer, const char *image_path, SDL_Rect *l_dest_rect);

#endif // RENDERING_H