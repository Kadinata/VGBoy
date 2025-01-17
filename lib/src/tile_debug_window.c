#include "tile_debug_window.h"

#include <stdint.h>
#include <string.h>

#include "bus_interface.h"
#include "color.h"
#include "status_code.h"
#include "window_manager.h"

typedef struct
{
  window_handle_t window;
  bus_interface_t data_bus_interface;
} window_ctx_t;

static const uint8_t scale = 4;
static const uint8_t tile_col_num = 16;
static const uint8_t tile_row_num = 24;
static const uint8_t tile_size = 8;
static const uint8_t padding = 1;

static window_ctx_t window_ctx;

static const color_rgba_t tile_colors[4] = {
    {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF},
    {.r = 0xAA, .g = 0xAA, .b = 0xAA, .a = 0xFF},
    {.r = 0x55, .g = 0x55, .b = 0x55, .a = 0xFF},
    {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF},
};

static void render_tile(SDL_Surface *surface, uint16_t start_address, uint16_t tileNum, uint16_t x, uint16_t y);
static inline uint8_t get_color_index(uint8_t msb, uint8_t lsb, uint8_t index);

status_code_t tile_debug_window_init(bus_interface_t const data_bus_interface)
{
  status_code_t status = STATUS_OK;

  window_init_param_t tile_debug_window_init_params = {
      .width = tile_col_num * (tile_size + padding) + padding,
      .height = tile_row_num * (tile_size + padding) + padding,
      .scale = scale,
  };

  memcpy(&window_ctx.data_bus_interface, &data_bus_interface, sizeof(bus_interface_t));

  status = window_init("Tilemap Debug Window", &window_ctx.window, &tile_debug_window_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

void tile_debug_window_update(void)
{
  uint16_t addr = 0x8000;
  uint16_t tile_num = 0;
  color_rgba_t background_color = {.r = 0x11, .g = 0x11, .b = 0x11, .a = 0xFF};
  SDL_Rect rect;

  rect.x = 0;
  rect.y = 0;
  rect.w = window_ctx.window.screen->w;
  rect.h = window_ctx.window.screen->h;

  SDL_FillRect(window_ctx.window.screen, &rect, background_color.as_hex);

  for (uint8_t row = 0; row < tile_row_num; row++)
  {
    for (uint8_t col = 0; col < tile_col_num; col++)
    {
      render_tile(window_ctx.window.screen, addr, tile_num++, row, col);
    }
  }

  SDL_UpdateTexture(window_ctx.window.texture, NULL, window_ctx.window.screen->pixels, window_ctx.window.screen->pitch);
  SDL_RenderClear(window_ctx.window.renderer);
  SDL_RenderCopy(window_ctx.window.renderer, window_ctx.window.texture, NULL, NULL);
  SDL_RenderPresent(window_ctx.window.renderer);
}

void tile_debug_window_cleanup(void)
{
  window_destroy(&window_ctx.window);
}

static void render_tile(SDL_Surface *surface, uint16_t start_address, uint16_t tile_num, uint16_t row, uint16_t col)
{
  SDL_Rect rect;

  for (uint8_t tile_col = 0; tile_col < 16; tile_col += 2)
  {
    uint8_t lsb, msb;
    bus_interface_read(&window_ctx.data_bus_interface, start_address + (tile_num * 16) + tile_col, &lsb);
    bus_interface_read(&window_ctx.data_bus_interface, start_address + (tile_num * 16) + tile_col + 1, &msb);

    for (uint8_t index = 0; index < 8; index++)
    {
      rect.x = ((col * 9 + padding) + index) * scale;
      rect.y = ((row * 9 + padding) + (tile_col / 2)) * scale;
      rect.w = scale;
      rect.h = scale;

      SDL_FillRect(surface, &rect, tile_colors[get_color_index(msb, lsb, index)].as_hex);
    }
  }
}

static inline uint8_t get_color_index(uint8_t msb, uint8_t lsb, uint8_t index)
{
  msb = ((msb << index) >> 7) & 0x1;
  lsb = ((lsb << index) >> 7) & 0x1;
  return ((msb << 1) | lsb) & 0x3;
}
