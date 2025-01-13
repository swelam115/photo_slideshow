#ifndef RENDERING_H
#define RENDERING_H

#include <SDL2/SDL.h>

// Function to render an image with a specific alpha value
void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha);

// Function to load an image and create a texture
SDL_Texture *load_image(SDL_Renderer *renderer, const char *image_path, SDL_Rect *l_dest_rect);

#endif // RENDERING_H