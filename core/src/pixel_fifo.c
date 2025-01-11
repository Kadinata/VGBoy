#include "pixel_fifo.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "ring_buf.h"
#include "bus_interface.h"
#include "pixel_fetcher.h"
#include "status_code.h"

#define PIXELS_PER_TILE (8)

static status_code_t pxfifo_shift_in(pxfifo_handle_t *const pxfifo);
static status_code_t pxfifo_shift_out(pxfifo_handle_t *const pxfifo, pixel_data_t *const pixel_out);
static status_code_t pxfifo_mix_sprite_pixel(pxfifo_handle_t *const pxfifo, pxfifo_item_t *const bgw_pixel);
static status_code_t handle_pxfifo_push_data(pxfifo_handle_t *const pxfifo);
static inline bool on_a_window(lcd_handle_t *const lcd, uint8_t x_coord);

status_code_t pxfifo_init(pxfifo_handle_t *const pxfifo, pxfifo_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->bus_interface, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->lcd, STATUS_ERR_INVALID_ARG);

  VERIFY_COND_RETURN_STATUS_IF_TRUE(
      !INIT_RING_BUFFER(pxfifo->bg_fifo.buffer, pxfifo->bg_fifo.storage),
      STATUS_ERR_GENERIC);

  pxfifo->fifo_state = PXFIFO_GET_TILE_NUM;
  pxfifo->lcd = param->lcd;
  memset(&pxfifo->counters, 0, sizeof(pxfifo_counter_t));
  memcpy(&pxfifo->bus_interface, param->bus_interface, sizeof(bus_interface_t));

  return STATUS_OK;
}

status_code_t pxfifo_reset(pxfifo_handle_t *const pxfifo)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);

  status_code_t status = STATUS_OK;
  status = pixel_fetcher_reset(&pxfifo->pixel_fetcher);
  RETURN_STATUS_IF_NOT_OK(status);

  memset(&pxfifo->counters, 0, sizeof(pxfifo_counter_t));
  pxfifo->fifo_state = PXFIFO_GET_TILE_NUM;
  ring_buffer_flush(&pxfifo->bg_fifo.buffer);

  return STATUS_OK;
}

status_code_t pxfifo_shift_pixel(pxfifo_handle_t *const pxfifo, pixel_data_t *const pixel_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pixel_out);

  status_code_t status = STATUS_OK;

  status = pxfifo_shift_in(pxfifo);
  RETURN_STATUS_IF_NOT_OK(status);

  status = pxfifo_shift_out(pxfifo, pixel_out);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t pxfifo_shift_in(pxfifo_handle_t *const pxfifo)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);

  status_code_t status = STATUS_OK;
  pixel_fetcher_context_t fetcher_ctx = (pixel_fetcher_context_t){
      .fetcher_state = &pxfifo->pixel_fetcher,
      .bus_interface = &pxfifo->bus_interface,
      .lcd_handle = pxfifo->lcd,
  };

  switch (pxfifo->fifo_state)
  {
  case PXFIFO_GET_TILE_NUM:
    if (pxfifo->counters.ticks & 1)
    {
      status = fetch_tile_number(&fetcher_ctx);
      pxfifo->fifo_state = PXFIFO_GET_DATA_LOW;
    }
    break;

  case PXFIFO_GET_DATA_LOW:
    if (pxfifo->counters.ticks & 1)
    {
      status = fetch_tile_data(&fetcher_ctx, 0);
      pxfifo->fifo_state = PXFIFO_GET_DATA_HIGH;
    }
    break;

  case PXFIFO_GET_DATA_HIGH:
    if (pxfifo->counters.ticks & 1)
    {
      status = fetch_tile_data(&fetcher_ctx, 1);
      pxfifo->fifo_state = PXFIFO_SLEEP;
    }
    break;

  case PXFIFO_SLEEP:
    if (pxfifo->counters.ticks & 1)
    {
      pxfifo->fifo_state = PXFIFO_PUSH;
    }
    break;

  case PXFIFO_PUSH:
    status = handle_pxfifo_push_data(pxfifo);
    break;

  default:
    break;
  }

  RETURN_STATUS_IF_NOT_OK(status);

  pxfifo->counters.ticks++;
  return STATUS_OK;
}

static status_code_t pxfifo_shift_out(pxfifo_handle_t *const pxfifo, pixel_data_t *const pixel_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pixel_out);

  pxfifo_item_t fifo_item;
  status_code_t status = STATUS_OK;

  pixel_out->data_valid = 0;

  if (ring_buffer_size(&pxfifo->bg_fifo.buffer) <= 8)
  {
    return STATUS_OK;
  }

  if (!ring_buffer_read(&pxfifo->bg_fifo.buffer, &fifo_item))
  {
    return STATUS_ERR_GENERIC;
  }

  if (pxfifo->counters.popped_px >= (pxfifo->lcd->registers.scroll_x % 8) || on_a_window(pxfifo->lcd, pxfifo->counters.popped_px))
  {
    status = lcd_get_palette_color(pxfifo->lcd, fifo_item.palette, fifo_item.pixel_color, &pixel_out->color);
    RETURN_STATUS_IF_NOT_OK(status);

    pixel_out->screen_x = pxfifo->counters.render_px;
    pixel_out->screen_y = pxfifo->lcd->registers.ly;
    pixel_out->data_valid = 1;

    pxfifo->counters.render_px++;
  }

  pxfifo->counters.popped_px++;

  return STATUS_OK;
}

static status_code_t pxfifo_mix_sprite_pixel(pxfifo_handle_t *const pxfifo, pxfifo_item_t *const bgw_pixel)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pxfifo);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bgw_pixel);

  pixel_fetcher_state_t *const fetcher_state = &pxfifo->pixel_fetcher;

  for (uint8_t i = 0; i < fetcher_state->fetched_sprite_count; i++)
  {
    fetched_sprite_tile_t *const current_sprite = &fetcher_state->sprite_tile_data[i];

    int16_t sprite_x = (current_sprite->tile_entry.x_pos - 8) + (pxfifo->lcd->registers.scroll_x % 8);

    if (sprite_x + 8 < pxfifo->counters.pushed_px)
    {
      continue;
    }

    int16_t offset = pxfifo->counters.pushed_px - sprite_x;

    if (offset < 0 || offset > 7)
    {
      continue;
    }

    uint8_t sprite_pixel_color = sprite_pixel_color_index(current_sprite, offset);
    uint8_t bg_priority = current_sprite->tile_entry.attrs & OAM_ATTR_BG_PRIORITY;

    if (sprite_pixel_color && (!bg_priority || bgw_pixel->pixel_color == 0))
    {
      // Sprite is not transparent and the background does not have priority or is transparent
      bgw_pixel->palette = (current_sprite->tile_entry.attrs & OAM_ATTR_DMG_PALETTE_NUM) ? PALETTE_OBJ_1 : PALETTE_OBJ_0;
      bgw_pixel->pixel_color = sprite_pixel_color;
      break;
    }
  }

  return STATUS_OK;
}

static status_code_t handle_pxfifo_push_data(pxfifo_handle_t *const pxfifo)
{
  if (ring_buffer_size(&pxfifo->bg_fifo.buffer) > 8)
  {
    return STATUS_OK;
  }

  status_code_t status = STATUS_OK;
  int16_t x = (pxfifo->pixel_fetcher.fetcher_x_index * 8) - (8 - (pxfifo->lcd->registers.scroll_x % 8));

  for (int8_t i = 0; i < PIXELS_PER_TILE; i++)
  {
    pxfifo_item_t fifo_item = {
        .pixel_color = bgw_pixel_color_index(&pxfifo->pixel_fetcher.bgw_tile_data, i),
        .obj_priority = 0,
        .bg_priority = 0,
        .palette = PALETTE_BGW,
    };

    if (!(pxfifo->lcd->registers.lcd_ctrl & LCD_CTRL_BGW_EN))
    {
      fifo_item.pixel_color = 0;
    }

    if (pxfifo->lcd->registers.lcd_ctrl & LCD_CTRL_OBJ_EN)
    {
      // Pre-mix background / window pixel with sprite pixels to eliminate the need to use 2 FIFOs
      status = pxfifo_mix_sprite_pixel(pxfifo, &fifo_item);
      RETURN_STATUS_IF_NOT_OK(status);
    }

    if (x >= 0)
    {
      ring_buffer_write(&pxfifo->bg_fifo.buffer, &fifo_item);
      pxfifo->counters.pushed_px++;
    }
  }

  pxfifo->counters.ticks = 0;
  pxfifo->fifo_state = PXFIFO_GET_TILE_NUM;

  return STATUS_OK;
}

static inline bool on_a_window(lcd_handle_t *const lcd, uint8_t x_coord)
{
  return (lcd_window_enabled(lcd) && (lcd->registers.ly >= lcd->registers.window_y) && (x_coord >= (lcd->registers.window_x - 7)));
}