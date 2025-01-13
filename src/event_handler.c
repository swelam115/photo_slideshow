#include "event_handler.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <pthread.h>

extern bool running;
extern bool next_pressed;
extern bool back_pressed;
extern bool paused;
extern pthread_mutex_t mutex;

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
