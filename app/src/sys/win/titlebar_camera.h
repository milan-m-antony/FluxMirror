#ifndef SC_TITLEBAR_CAMERA_H
#define SC_TITLEBAR_CAMERA_H

#include <stdbool.h>
#include <SDL3/SDL.h>

void
sc_titlebar_camera_init(SDL_Window *window, const char *serial);

void
sc_titlebar_camera_destroy(void);

#endif
