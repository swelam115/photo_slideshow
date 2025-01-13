#include "rendering.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>

extern int window_width;
extern int window_height;

#define MAX_TEXTURE_SIZE 2048

void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha) {
    SDL_SetTextureAlphaMod(l_texture, alpha);
    SDL_RenderCopy(renderer, l_texture, NULL, l_dest_rect);
}

SDL_Texture *load_image(SDL_Renderer *renderer, const char *image_path, SDL_Rect *l_dest_rect) {
    SDL_Surface *surface = IMG_Load(image_path);
    if (!surface) {
        fprintf(stderr, "IMG_Load Error: %s\n", IMG_GetError());
        return NULL;
    }

    int original_width = surface->w;
    int original_height = surface->h;

    if (original_width > MAX_TEXTURE_SIZE || original_height > MAX_TEXTURE_SIZE) {
        double scale_factor = (double)MAX_TEXTURE_SIZE / (original_width > original_height ? original_width : original_height);
        int new_width = (int)(original_width * scale_factor);
        int new_height = (int)(original_height * scale_factor);

        SDL_Surface *resized_surface = SDL_CreateRGBSurfaceWithFormat(0, new_width, new_height, 32, SDL_PIXELFORMAT_RGBA32);
        if (!resized_surface) {
            fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat Error: %s\n", SDL_GetError());
            SDL_FreeSurface(surface);
            return NULL;
        }

        SDL_BlitScaled(surface, NULL, resized_surface, NULL);
        SDL_FreeSurface(surface);
        surface = resized_surface;
    }

    double aspect_ratio = (double)surface->w / (double)surface->h;
    int target_width;
    int target_height;

    if (aspect_ratio >= 1.0) { // Landscape or square
        target_width = window_width;
        target_height = (int)(window_width / aspect_ratio);
        if (target_height > window_height) {
            target_height = window_height;
            target_width = (int)(window_height * aspect_ratio);
        }
    } else { // Portrait
        target_height = window_height;
        target_width = (int)(window_height * aspect_ratio);
        if (target_width > window_width) {
            target_width = window_width;
            target_height = (int)(window_width / aspect_ratio);
        }
    }

    l_dest_rect->x = (window_width - target_width) / 2;
    l_dest_rect->y = (window_height - target_height) / 2;
    l_dest_rect->w = target_width;
    l_dest_rect->h = target_height;

    SDL_Texture *l_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!l_texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        return NULL;
    }

    return l_texture;
}