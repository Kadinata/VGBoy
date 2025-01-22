#include "main_window.h"

#include <stdint.h>

#include "status_code.h"
#include "window_manager.h"

typedef struct
{
  window_handle_t window;
  uint32_t *video_buffer;
  uint16_t window_width;
  uint16_t window_height;
} window_ctx_t;

static window_ctx_t window_ctx;
static const uint8_t scale = 4;

status_code_t main_window_init(uint32_t *const video_buffer, uint16_t window_width, uint16_t window_height)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(video_buffer);

  status_code_t status = STATUS_OK;

  window_init_param_t main_window_init_params = {
      .width = window_width,
      .height = window_height,
      .scale = scale,
  };

  window_ctx.video_buffer = video_buffer;
  window_ctx.window_width = window_width;
  window_ctx.window_height = window_height;

  status = window_init("Gameboy Emulator", &window_ctx.window, &main_window_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

void main_window_update(void)
{
  SDL_Rect rc;
  rc.x = 0;
  rc.y = 0;
  rc.w = window_ctx.window.screen->w;
  rc.h = window_ctx.window.screen->h;

  for (uint8_t row = 0; row < window_ctx.window_height; row++)
  {
    for (uint8_t col = 0; col < window_ctx.window_width; col++)
    {
      rc.x = col * scale;
      rc.y = row * scale;
      rc.w = scale;
      rc.h = scale;

      SDL_FillRect(window_ctx.window.screen, &rc, window_ctx.video_buffer[col + (row * window_ctx.window_width)]);
    }
  }

  SDL_UpdateTexture(window_ctx.window.texture, NULL, window_ctx.window.screen->pixels, window_ctx.window.screen->pitch);
  SDL_RenderClear(window_ctx.window.renderer);
  SDL_RenderCopy(window_ctx.window.renderer, window_ctx.window.texture, NULL, NULL);
  SDL_RenderPresent(window_ctx.window.renderer);
}

void main_window_cleanup(void)
{
  window_destroy(&window_ctx.window);
}
