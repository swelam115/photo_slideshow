#include "rendering.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <libexif/exif-data.h>

// External variables for window dimensions
extern int window_width;
extern int window_height;

#define MAX_TEXTURE_SIZE 2048 // Maximum texture size

// Function to render an image with a specific alpha value
void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha) {
    SDL_SetTextureAlphaMod(l_texture, alpha); // Set the alpha value
    SDL_RenderCopy(renderer, l_texture, NULL, l_dest_rect); // Render the texture
}

// Function to check JPG image orientation
int get_exif_orientation(const char* filename) {
    ExifData* ed = exif_data_new_from_file(filename);
    if (!ed) return 1; // Default orientation

    ExifEntry* entry = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);
    if (entry && entry->format == EXIF_FORMAT_SHORT && entry->data) {
        int orientation = exif_get_short(entry->data, exif_data_get_byte_order(ed));
        exif_data_unref(ed);
        return orientation;
    }
    exif_data_unref(ed);
    return 1; // Default orientation
}

// Function to rotate an image based on its orientation
SDL_Surface* rotate_image(SDL_Surface* surface, int orientation) {
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    double angle = 0;

    switch (orientation) {
        case 2: flip = SDL_FLIP_HORIZONTAL; break;
        case 3: angle = 180; break;
        case 4: flip = SDL_FLIP_VERTICAL; break;
        case 5: flip = SDL_FLIP_HORIZONTAL; angle = 90; break;
        case 6: angle = 90; break;
        case 7: flip = SDL_FLIP_HORIZONTAL; angle = -90; break;
        case 8: angle = -90; break;
        default: break; // Orientation 1: No transformation
    }

    // Create a texture for rendering
    SDL_Renderer* renderer = SDL_CreateRenderer(SDL_CreateWindow("Transform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, surface->w, surface->h, 0), -1, 0);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Render to a new surface with the correct orientation
    SDL_Surface* transformed = SDL_CreateRGBSurfaceWithFormat(0, surface->w, surface->h, 32, surface->format->format);
    SDL_SetRenderTarget(renderer, SDL_CreateTextureFromSurface(renderer, transformed));
    SDL_RenderCopyEx(renderer, texture, NULL, NULL, angle, NULL, flip);

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);

    return transformed;
}

// Function to load an image and create a texture
SDL_Texture *load_image(SDL_Renderer *renderer, const char *image_path, SDL_Rect *l_dest_rect) {
    SDL_Surface *surface = IMG_Load(image_path); // Load the image
    if (!surface) { // Check if image loading was successful
        fprintf(stderr, "IMG_Load Error: %s\n", IMG_GetError());
        return NULL;
    }

    if (strstr(image_path, ".jpg") || strstr(image_path, ".JPG") || strstr(image_path, ".jpeg") || strstr(image_path, ".JPEG")) {
        int orientation = get_exif_orientation(image_path);
        if (orientation != 1) {
            surface = rotate_image(surface, orientation);
        }
    }

    int original_width = surface->w;
    int original_height = surface->h;

    // Resize the image if it exceeds the maximum texture size
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

        SDL_BlitScaled(surface, NULL, resized_surface, NULL); // Scale the image
        SDL_FreeSurface(surface); // Free the original surface
        surface = resized_surface; // Use the resized surface
    }

    // Calculate the destination rectangle dimensions
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

    // Set the destination rectangle
    l_dest_rect->x = (window_width - target_width) / 2;
    l_dest_rect->y = (window_height - target_height) / 2;
    l_dest_rect->w = target_width;
    l_dest_rect->h = target_height;

    SDL_Texture *l_texture = SDL_CreateTextureFromSurface(renderer, surface); // Create the texture
    SDL_FreeSurface(surface); // Free the surface

    if (!l_texture) { // Check if texture creation was successful
        fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
        return NULL;
    }

    return l_texture; // Return the texture
}