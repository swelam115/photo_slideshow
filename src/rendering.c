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
// Function to manually rotate an SDL_Surface based on its orientation
SDL_Surface* rotate_image(SDL_Surface* surface, int orientation) {
    SDL_Surface* rotated = NULL;
    int w = surface->w;
    int h = surface->h;
    
    switch (orientation) {
        case 3: // 180 degrees
            rotated = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, surface->format->format);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    ((Uint32*)rotated->pixels)[(h - y - 1) * w + (w - x - 1)] = ((Uint32*)surface->pixels)[y * w + x];
                }
            }
            break;
        case 6: // 90 degrees clockwise
            rotated = SDL_CreateRGBSurfaceWithFormat(0, h, w, 32, surface->format->format);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    ((Uint32*)rotated->pixels)[x * h + (h - y - 1)] = ((Uint32*)surface->pixels)[y * w + x];
                }
            }
            break;
        case 8: // 90 degrees counterclockwise
            rotated = SDL_CreateRGBSurfaceWithFormat(0, h, w, 32, surface->format->format);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    ((Uint32*)rotated->pixels)[(w - x - 1) * h + y] = ((Uint32*)surface->pixels)[y * w + x];
                }
            }
            break;
        default: // Orientation 1: No transformation
            rotated = SDL_ConvertSurface(surface, surface->format, 0);
            break;
    }

    return rotated;
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