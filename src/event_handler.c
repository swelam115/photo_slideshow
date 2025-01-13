#include "event_handler.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <pthread.h>

// External variables shared with other parts of the program
extern bool running;
extern bool next_pressed;
extern bool back_pressed;
extern bool paused;
extern pthread_mutex_t mutex;

// Event handler thread function
void *event_handler_thread() {
    while (running) { // Loop while the program is running
        SDL_Event event;
        while (SDL_PollEvent(&event)) { // Poll for events
            if (event.type == SDL_QUIT) { // Handle quit event
                pthread_mutex_lock(&mutex);
                running = 0; // Set running to false to stop the program
                pthread_mutex_unlock(&mutex);
                return NULL;
            } else if (event.type == SDL_KEYDOWN) { // Handle keydown event
                pthread_mutex_lock(&mutex); // Lock the mutex
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        next_pressed = 1; // Set next_pressed flag
                        break;
                    case SDLK_LEFT:
                        back_pressed = 1; // Set back_pressed flag
                        break;
                    case SDLK_SPACE:
                        paused = !paused; // Toggle paused flag
                        break;
                }
                pthread_mutex_unlock(&mutex); // Unlock the mutex
            }
        }
        SDL_Delay(10); // Delay to avoid busy-waiting
    }
    return NULL;
}