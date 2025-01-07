#ifndef __DMG_BG_TILE_FETCHER_H__
#define __DMG_BG_TILE_FETCHER_H__

#include <stdint.h>

#include "bus_interface.h"
#include "lcd.h"
#include "oam.h"
#include "status_code.h"

#define MAX_FETCHED_SPRITES (10)

typedef struct
{
  uint8_t tile_num;
  uint8_t tile_data_low;
  uint8_t tile_data_high;
} fetched_bgw_tile_t;

typedef struct
{
  oam_entry_t tile_entry;
  uint8_t tile_data_low;
  uint8_t tile_data_high;
} fetched_sprite_tile_t;

typedef struct
{
  uint8_t fetcher_x_index;
  uint8_t window_line;
  uint8_t fetched_sprite_count;
  fetched_bgw_tile_t bgw_tile_data;
  fetched_sprite_tile_t sprite_tile_data[MAX_FETCHED_SPRITES];
  oam_scanned_sprites_t oam_scanned_sprites;
} pixel_fetcher_state_t;

typedef struct
{
  pixel_fetcher_state_t *fetcher_state;
  lcd_handle_t *lcd_handle;
  bus_interface_t *bus_interface;
} pixel_fetcher_context_t;

status_code_t pixel_fetcher_reset(pixel_fetcher_state_t *const state);
status_code_t fetch_tile_number(pixel_fetcher_context_t *const ctx);
status_code_t fetch_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset);
uint8_t bgw_pixel_color_index(fetched_bgw_tile_t *const bgw_tile, uint8_t index);
uint8_t sprite_pixel_color_index(fetched_sprite_tile_t *const sprite_tile, uint8_t index);

#endif /* __DMG_BG_TILE_FETCHER_H__ */
