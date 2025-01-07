#include "pixel_fetcher.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "lcd.h"
#include "oam.h"
#include "bus_interface.h"
#include "status_code.h"

#define VERIFY_CTX_RETURN_STATUS_IF_ERROR(ctx)                                    \
  {                                                                               \
    VERIFY_PTR_RETURN_ERROR_IF_NULL(ctx);                                         \
    VERIFY_PTR_RETURN_STATUS_IF_NULL(ctx->fetcher_state, STATUS_ERR_INVALID_ARG); \
    VERIFY_PTR_RETURN_STATUS_IF_NULL(ctx->lcd_handle, STATUS_ERR_INVALID_ARG);    \
    VERIFY_PTR_RETURN_STATUS_IF_NULL(ctx->bus_interface, STATUS_ERR_INVALID_ARG); \
  }

static status_code_t fetch_bg_tile_num(pixel_fetcher_context_t *const ctx);
static status_code_t fetch_window_tile_num(pixel_fetcher_context_t *const ctx);
static status_code_t fetch_sprite_tile_num(pixel_fetcher_context_t *const ctx);
static status_code_t fetch_bg_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset);
static status_code_t fetch_sprite_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset);
static inline bool window_is_in_view(lcd_handle_t *lcd_handle, pixel_fetcher_state_t *fetcher_state);
static inline bool sprite_is_in_view(oam_entry_t *oam_entry, lcd_handle_t *lcd_handle, pixel_fetcher_state_t *fetcher_state);

status_code_t pixel_fetcher_reset(pixel_fetcher_state_t *const state)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(state);

  state->fetcher_x_index = 0;

  return STATUS_OK;
}

status_code_t fetch_tile_number(pixel_fetcher_context_t *const ctx)
{
  VERIFY_CTX_RETURN_STATUS_IF_ERROR(ctx);

  status_code_t status = STATUS_OK;

  if (ctx->lcd_handle->registers.lcd_ctrl & LCD_CTRL_BGW_EN)
  {
    status = fetch_bg_tile_num(ctx);
    RETURN_STATUS_IF_NOT_OK(status);

    status = fetch_window_tile_num(ctx);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  status = fetch_sprite_tile_num(ctx);
  RETURN_STATUS_IF_NOT_OK(status);

  ctx->fetcher_state->fetcher_x_index++;

  return STATUS_OK;
}

status_code_t fetch_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset)
{
  VERIFY_CTX_RETURN_STATUS_IF_ERROR(ctx);

  status_code_t status = STATUS_OK;

  status = fetch_bg_tile_data(ctx, offset);
  RETURN_STATUS_IF_NOT_OK(status);

  status = fetch_sprite_tile_data(ctx, offset);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

uint8_t bgw_pixel_color_index(fetched_bgw_tile_t *const bgw_tile, uint8_t index)
{
  if (bgw_tile == NULL)
  {
    return 0;
  }
  uint8_t msb = ((bgw_tile->tile_data_high << index) >> 7) & 0x1;
  uint8_t lsb = ((bgw_tile->tile_data_low << index) >> 7) & 0x1;

  return (msb << 1) | lsb;
}

uint8_t sprite_pixel_color_index(fetched_sprite_tile_t *const sprite_tile, uint8_t index)
{
  if (sprite_tile == 0)
  {
    return 0;
  }

  if (sprite_tile->tile_entry.attrs & OAM_ATTR_X_FLIP)
  {
    index = 7 - index;
  }

  uint8_t msb = ((sprite_tile->tile_data_high << index) >> 7) & 0x1;
  uint8_t lsb = ((sprite_tile->tile_data_low << index) >> 7) & 0x1;

  return (msb << 1) | lsb;
}

static status_code_t fetch_bg_tile_num(pixel_fetcher_context_t *const ctx)
{
  status_code_t status = STATUS_OK;
  lcd_handle_t *const lcd_handle = ctx->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = ctx->fetcher_state;

  uint16_t address = lcd_ctrl_bg_tile_map_address(lcd_handle);
  address += (fetcher_state->fetcher_x_index + (lcd_handle->registers.scroll_x / 8)) & 0x1F;
  address += (((lcd_handle->registers.ly + lcd_handle->registers.scroll_y) & 0xFF) / 8) * 32;

  status = bus_interface_read(ctx->bus_interface, address, &fetcher_state->bgw_tile_data.tile_num);
  RETURN_STATUS_IF_NOT_OK(status);

  if (lcd_ctrl_bgw_tile_data_address(lcd_handle) == 0x8800)
  {
    fetcher_state->bgw_tile_data.tile_num += 0x80;
  }

  return STATUS_OK;
}

static status_code_t fetch_window_tile_num(pixel_fetcher_context_t *const ctx)
{
  status_code_t status = STATUS_OK;
  lcd_handle_t *const lcd_handle = ctx->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = ctx->fetcher_state;

  if (!lcd_window_visible(lcd_handle) || !window_is_in_view(lcd_handle, fetcher_state))
  {
    return STATUS_OK;
  }

  uint16_t address = lcd_ctrl_window_tile_map_address(lcd_handle);
  address += ((fetcher_state->fetcher_x_index * 8) + 7 - lcd_handle->registers.window_x) / 8;
  address += (fetcher_state->window_line / 8) * 32;

  status = bus_interface_read(ctx->bus_interface, address, &fetcher_state->bgw_tile_data.tile_num);
  RETURN_STATUS_IF_NOT_OK(status);

  if (lcd_ctrl_bgw_tile_data_address(lcd_handle) == 0x8800)
  {
    fetcher_state->bgw_tile_data.tile_num += 0x80;
  }

  return STATUS_OK;
}

static status_code_t fetch_sprite_tile_num(pixel_fetcher_context_t *const ctx)
{
  lcd_handle_t *const lcd_handle = ctx->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = ctx->fetcher_state;

  fetcher_state->fetched_sprite_count = 0;
  if (!(lcd_handle->registers.lcd_ctrl & LCD_CTRL_OBJ_EN))
  {
    return STATUS_OK;
  }

  uint8_t index = 0;

  while ((index < fetcher_state->oam_scanned_sprites.sprite_count) && (fetcher_state->fetched_sprite_count < MAX_FETCHED_SPRITES))
  {
    oam_entry_t *entry = &fetcher_state->oam_scanned_sprites.sprite_attributes[index++];

    if (sprite_is_in_view(entry, lcd_handle, fetcher_state))
    {
      fetcher_state->sprite_tile_data[fetcher_state->fetched_sprite_count++].tile_entry = *entry;
    }
  }

  return STATUS_OK;
}

static status_code_t fetch_bg_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset)
{
  status_code_t status = STATUS_OK;
  lcd_handle_t *const lcd_handle = ctx->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = ctx->fetcher_state;

  uint8_t *const target = !!offset ? &fetcher_state->bgw_tile_data.tile_data_high : &fetcher_state->bgw_tile_data.tile_data_low;

  uint16_t address = lcd_ctrl_bgw_tile_data_address(lcd_handle);
  address += fetcher_state->bgw_tile_data.tile_num * 16;
  address += ((lcd_handle->registers.ly + lcd_handle->registers.scroll_y) % 8) * 2;
  address += !!offset;

  status = bus_interface_read(ctx->bus_interface, address, target);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t fetch_sprite_tile_data(pixel_fetcher_context_t *const ctx, uint8_t const offset)
{
  status_code_t status = STATUS_OK;
  lcd_handle_t *const lcd_handle = ctx->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = ctx->fetcher_state;

  uint8_t sprite_height = lcd_ctrl_obj_size(lcd_handle);

  for (uint8_t i = 0; i < fetcher_state->fetched_sprite_count; i++)
  {
    fetched_sprite_tile_t *current_sprite = &fetcher_state->sprite_tile_data[i];

    uint8_t tile_index = current_sprite->tile_entry.tile;
    uint8_t tile_y = ((lcd_handle->registers.ly + 16) - current_sprite->tile_entry.y_pos) * 2;

    // Check if the sprite is vertically flipped
    if (current_sprite->tile_entry.attrs & OAM_ATTR_Y_FLIP)
    {
      tile_y = ((sprite_height * 2) - 2) - tile_y;
    }

    if (sprite_height == 16)
    {
      tile_index &= ~0x1;
    }

    uint8_t *const target = !!offset ? &current_sprite->tile_data_high : &current_sprite->tile_data_low;

    uint16_t address = 0x8000 + (tile_index * 16) + tile_y + !!offset;
    status = bus_interface_read(ctx->bus_interface, address, target);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static inline bool window_is_in_view(lcd_handle_t *lcd_handle, pixel_fetcher_state_t *fetcher_state)
{
  if (!lcd_handle || !fetcher_state)
  {
    return false;
  }

  uint8_t wy = lcd_handle->registers.window_y;
  uint8_t wx = lcd_handle->registers.window_x;
  uint8_t line_x = (fetcher_state->fetcher_x_index * 8) + 7;
  uint8_t line_y = lcd_handle->registers.ly;

  return ((line_x >= wx) && (line_x < wx + SCREEN_WIDTH + 14) && (line_y >= wy) && (line_y < wy + SCREEN_HEIGHT));
}

static inline bool sprite_is_in_view(oam_entry_t *oam_entry, lcd_handle_t *lcd_handle, pixel_fetcher_state_t *fetcher_state)
{
  if (!oam_entry || !lcd_handle || !fetcher_state)
  {
    return false;
  }

  uint8_t line_x = fetcher_state->fetcher_x_index * 8;
  uint8_t sprite_x = (oam_entry->x_pos - 8) + (lcd_handle->registers.scroll_x % 8);

  return ((sprite_x >= line_x && sprite_x < line_x + 8) || ((sprite_x + 8) >= line_x && (sprite_x + 8) < line_x + 8));
}
