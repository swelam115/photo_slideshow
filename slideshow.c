#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

#define MAX_TEXTURE_SIZE 2048
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
int load_images(const char *directory, char ***image_paths);
void shuffle_images(char **image_paths, int count);
void free_images(char **image_paths, int count);
void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha);
SDL_Texture *load_image(SDL_Renderer *renderer, const char *image_path, SDL_Rect *l_dest_rect);
void *event_handler_thread();

// Handle changes triggered by next or back button presses
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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // Enable high-quality texture scaling

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

void *event_handler_thread() {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                pthread_mutex_lock(&mutex);
                running = 0;
                pthread_mutex_unlock(&mutex);
                return NULL;
            } else if (event.type == SDL_KEYDOWN) {
                pthread_mutex_lock(&mutex);
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        next_pressed = 1;
                        break;
                    case SDLK_LEFT:
                        back_pressed = 1;
                        break;
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                }
                pthread_mutex_unlock(&mutex);
            }
        }
        SDL_Delay(10);
    }
    return NULL;
}

// Function to load all image paths from a directory
int load_images(const char *directory, char ***image_paths) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    char **paths = NULL;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0)) {
                paths = realloc(paths, sizeof(char *) * (count + 1));
                if (!paths) {
                    perror("realloc");
                    closedir(dir);
                    return -1;
                }

                paths[count] = malloc(strlen(directory) + strlen(entry->d_name) + 2);
                if (!paths[count]) {
                    perror("malloc");
                    closedir(dir);
                    return -1;
                }

                sprintf(paths[count], "%s/%s", directory, entry->d_name);
                count++;
            }
        }
    }

    closedir(dir);
    *image_paths = paths;
    return count;
}

// Shuffle the loaded image paths
void shuffle_images(char **image_paths, int count) {
    srand((unsigned int)time(NULL));
    for (int i = count - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        char *temp = image_paths[i];
        image_paths[i] = image_paths[j];
        image_paths[j] = temp;
    }
}

// Free memory allocated for image paths
void free_images(char **image_paths, int count) {
    for (int i = 0; i < count; ++i) {
        free(image_paths[i]);
    }
    free(image_paths);
}

// Render the image with alpha transparency
void render_image(SDL_Renderer *renderer, SDL_Texture *l_texture, SDL_Rect *l_dest_rect, int alpha) {
    SDL_SetTextureAlphaMod(l_texture, alpha);
    SDL_RenderCopy(renderer, l_texture, NULL, l_dest_rect);
}

// Load an image into a texture and calculate its rendering dimensions
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
