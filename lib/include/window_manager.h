#ifndef __WINDOW_MANAGER_H__
#define __WINDOW_MANAGER_H__

#include <stdint.h>
#include <SDL2/SDL.h>
#include "status_code.h"

typedef struct
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_Surface *screen;
} window_handle_t;

typedef struct
{
  uint16_t width;
  uint16_t height;
  uint8_t scale;
} window_init_param_t;

status_code_t window_init(const char *title, window_handle_t *const handle, window_init_param_t *const param);
status_code_t window_destroy(window_handle_t *const handle);

#endif /* __WINDOW_MANAGER_H__ */
