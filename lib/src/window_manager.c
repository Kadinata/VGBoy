#include "window_manager.h"

#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "color.h"
#include "status_code.h"

typedef struct
{
  color_rgba_t r_mask;
  color_rgba_t g_mask;
  color_rgba_t b_mask;
  color_rgba_t a_mask;
} rgba_mask_t;

static rgba_mask_t rgba_mask = {
    .r_mask = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x00},
    .g_mask = {.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0x00},
    .b_mask = {.r = 0x00, .g = 0x00, .b = 0xFF, .a = 0x00},
    .a_mask = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF},
};

status_code_t window_init(const char *title, window_handle_t *const handle, window_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(title);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(param->width == 0, STATUS_ERR_INVALID_ARG);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(param->height == 0, STATUS_ERR_INVALID_ARG);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(param->scale == 0, STATUS_ERR_INVALID_ARG);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->screen != NULL, STATUS_ERR_ALREADY_INITIALIZED);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->texture != NULL, STATUS_ERR_ALREADY_INITIALIZED);

  uint16_t width = param->width * param->scale;
  uint16_t height = param->height * param->scale;

  handle->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->window == NULL, STATUS_ERR_GENERIC);

  handle->renderer = SDL_CreateRenderer(handle->window, -1, SDL_RENDERER_ACCELERATED);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->renderer == NULL, STATUS_ERR_GENERIC);

  handle->screen = SDL_CreateRGBSurface(
      0, width, height, 32,
      rgba_mask.r_mask.as_hex,
      rgba_mask.g_mask.as_hex,
      rgba_mask.b_mask.as_hex,
      rgba_mask.a_mask.as_hex);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->screen == NULL, STATUS_ERR_GENERIC);

  handle->texture = SDL_CreateTexture(
      handle->renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      width, height);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(handle->texture == NULL, STATUS_ERR_GENERIC);

  return STATUS_OK;
}

status_code_t window_destroy(window_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(handle->window, STATUS_ERR_ALREADY_FREED);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(handle->renderer, STATUS_ERR_ALREADY_FREED);

  SDL_DestroyRenderer(handle->renderer);
  SDL_DestroyWindow(handle->window);

  memset(handle, 0, sizeof(window_handle_t));
  return STATUS_OK;
}
