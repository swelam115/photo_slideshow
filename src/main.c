#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "image_handling.h"
#include "event_handler.h"
#include "rendering.h"

#define TRANSITION_TIME 1000 // Time for fade transitions in milliseconds
#define DISPLAY_TIME 5000   // Time to display each image in milliseconds

// Global variables for window dimensions
int window_width = 0;
int window_height = 0;
int current_index = 0; // Index of the current image in the slideshow
bool paused = 0;       // Indicates if the slideshow is paused
bool running = 1;      // Indicates if the program is running
bool next_pressed = 0; // Indicates if the next button was pressed
bool back_pressed = 0; // Indicates if the back button was pressed

SDL_Texture *texture = NULL; // Global texture for the current image
SDL_Rect dest_rect;          // Rectangle to define the rendered image dimensions

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to ensure thread-safe access to shared variables

// Function prototypes
void handle_change(SDL_Renderer *renderer, char **image_paths, int image_count);
void handle_pause(SDL_Renderer *renderer, char **image_paths, int image_count);

void handle_change(SDL_Renderer *renderer, char **image_paths, int image_count) {
    pthread_mutex_lock(&mutex);
    // Update the current index based on button press
    if (next_pressed) {
        current_index = (current_index + 1) % image_count;
        next_pressed = 0;
    } else if (back_pressed) {
        current_index = (current_index - 1 + image_count) % image_count;
        back_pressed = 0;
    }

    // Free the previous texture if it exists
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    // Load the new image into the texture
    texture = load_image(renderer, image_paths[current_index], &dest_rect);
    bool local_paused = paused;
    pthread_mutex_unlock(&mutex);

    // If paused, display the current image without transitioning
    if (local_paused) {
        SDL_RenderClear(renderer);
        render_image(renderer, texture, &dest_rect, 255);
        SDL_RenderPresent(renderer);
        return;
    }

    // Apply fade-in effect for the new image
    if (texture) {
        for (int alpha = 0; alpha <= 255; alpha += 15) {
            SDL_RenderClear(renderer);
            render_image(renderer, texture, &dest_rect, alpha);
            SDL_RenderPresent(renderer);
            SDL_Delay(TRANSITION_TIME / 34);
        }
    }
}

// Handle pause state when spacebar is pressed
void handle_pause(SDL_Renderer *renderer, char **image_paths, int image_count) {
    while (1) {
        pthread_mutex_lock(&mutex);
        bool local_paused = paused;
        bool local_next_pressed = next_pressed;
        bool local_back_pressed = back_pressed;
        pthread_mutex_unlock(&mutex);

        // Exit the loop if unpaused
        if (!local_paused) {
            break;
        }

        // Handle next or back button presses during pause
        if (local_next_pressed || local_back_pressed) {
            handle_change(renderer, image_paths, image_count);
        }

        SDL_Delay(100); // Delay to avoid busy-waiting
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_directory>\n", argv[0]);
        return 1;
    }

    const char *image_directory = argv[1];
    char **image_paths = NULL;

    fprintf(stdout, "Loading images from directory: %s\n", image_directory);
    int image_count = load_images(image_directory, &image_paths);

    if (image_count <= 0) {
        fprintf(stderr, "No images found in directory: %s\n", image_directory);
        return 1;
    }

    fprintf(stdout, "Loaded %d images. Shuffling images...\n", image_count);
    shuffle_images(image_paths, image_count);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        free_images(image_paths, image_count);
        return 1;
    }

    if (!IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG)) {
        fprintf(stderr, "IMG_Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        free_images(image_paths, image_count);
        return 1;
    }

    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(0, &display_mode) != 0) {
        fprintf(stderr, "SDL_GetCurrentDisplayMode Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        free_images(image_paths, image_count);
        return 1;
    }

    // Set window dimensions to match the display resolution
    window_width = display_mode.w;
    window_height = display_mode.h;

    fprintf(stdout, "Detected display resolution: %dx%d\n", window_width, window_height);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2"); // Enable high-quality texture scaling

    SDL_Window *window = SDL_CreateWindow("Slideshow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        free_images(image_paths, image_count);
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        free_images(image_paths, image_count);
        return 1;
    }

    pthread_t event_thread;
    // Create a thread to handle user input events
    if (pthread_create(&event_thread, NULL, event_handler_thread, NULL) != 0) {
        fprintf(stderr, "Error creating event handler thread\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        free_images(image_paths, image_count);
        return 1;
    }

    while (running) {
        texture = load_image(renderer, image_paths[current_index], &dest_rect);
        if (!texture) {
            fprintf(stderr, "Skipping image: %s\n", image_paths[current_index]);
            current_index = (current_index + 1) % image_count;
            continue;
        }

        // Apply fade-in effect for the current image
        for (int alpha = 0; alpha <= 255; alpha += 15) {
            pthread_mutex_lock(&mutex);
            bool local_paused = paused;
            bool local_next_pressed = next_pressed;
            bool local_back_pressed = back_pressed;
            pthread_mutex_unlock(&mutex);

            if (local_paused) {
                handle_pause(renderer, image_paths, image_count);
            }

            if (local_next_pressed || local_back_pressed) {
                handle_change(renderer, image_paths, image_count);
                break;
            }

            SDL_RenderClear(renderer);
            render_image(renderer, texture, &dest_rect, alpha);
            SDL_RenderPresent(renderer);
            SDL_Delay(TRANSITION_TIME / 17);
        }

        // Display the image for the specified duration
        for (int elapsed = 0; elapsed < DISPLAY_TIME; elapsed += 100) {
            pthread_mutex_lock(&mutex);
            bool local_paused = paused;
            bool local_next_pressed = next_pressed;
            bool local_back_pressed = back_pressed;
            pthread_mutex_unlock(&mutex);

            if (local_paused) {
                handle_pause(renderer, image_paths, image_count);
            }

            if (local_next_pressed || local_back_pressed) {
                handle_change(renderer, image_paths, image_count);
                elapsed = 0;
            }

            SDL_Delay(100);
        }

        current_index = (current_index + 1) % image_count; // Move to the next image
    }

    pthread_join(event_thread, NULL); // Wait for the event thread to finish

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    free_images(image_paths, image_count);

    fprintf(stdout, "Program terminated successfully.\n");
    return 0;
}
