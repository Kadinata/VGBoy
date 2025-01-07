#include "pixel_fifo.h"

#include <stdint.h>
#include <string.h>

#include "ring_buf.h"
#include "bus_interface.h"
#include "pixel_fetcher.h"
#include "status_code.h"

#define PIXELS_PER_TILE (8)

static status_code_t pxfifo_shift_in(pxfifo_handle_t *const handle);
static status_code_t pxfifo_shift_out(pxfifo_handle_t *const handle, pixel_data_t *const pixel_out);
static status_code_t pxfifo_mix_sprite_pixel(pxfifo_handle_t *const handle, pxfifo_item_t *const bgw_pixel);
static status_code_t handle_pxfifo_push_data(pxfifo_handle_t *const handle);

status_code_t pxfifo_init(pxfifo_handle_t *const handle, pxfifo_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->bus_interface, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(param->lcd_handle, STATUS_ERR_INVALID_ARG);

  VERIFY_COND_RETURN_STATUS_IF_TRUE(
      !INIT_RING_BUFFER(handle->bg_fifo.fifo_buffer, handle->bg_fifo.fifo_storage),
      STATUS_ERR_GENERIC);

  handle->fifo_state = PXFIFO_GET_TILE_NUM;
  handle->lcd_handle = param->lcd_handle;
  memset(&handle->ctx, 0, sizeof(pxfifo_context_t));
  memcpy(&handle->bus_interface, param->bus_interface, sizeof(bus_interface_t));

  return STATUS_OK;
}

status_code_t pxfifo_reset(pxfifo_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  status_code_t status = STATUS_OK;
  status = pixel_fetcher_reset(&handle->pixel_fetcher);
  RETURN_STATUS_IF_NOT_OK(status);

  handle->ctx.line_x = 0;
  handle->ctx.pushed_x = 0;
  handle->ctx.tick_count = 0;
  handle->ctx.fifo_x = 0;
  handle->fifo_state = PXFIFO_GET_TILE_NUM;
  ring_buffer_flush(&handle->bg_fifo.fifo_buffer);

  return STATUS_OK;
}

status_code_t pxfifo_shift_pixel(pxfifo_handle_t *const handle, pixel_data_t *const pixel_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pixel_out);

  status_code_t status = STATUS_OK;

  status = pxfifo_shift_in(handle);
  RETURN_STATUS_IF_NOT_OK(status);

  status = pxfifo_shift_out(handle, pixel_out);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t pxfifo_shift_in(pxfifo_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  status_code_t status = STATUS_OK;
  pixel_fetcher_context_t fetcher_ctx = (pixel_fetcher_context_t){
      .fetcher_state = &handle->pixel_fetcher,
      .bus_interface = &handle->bus_interface,
      .lcd_handle = handle->lcd_handle,
  };

  switch (handle->fifo_state)
  {
  case PXFIFO_GET_TILE_NUM:
    if (handle->ctx.tick_count & 1)
    {
      status = fetch_tile_number(&fetcher_ctx);
      handle->fifo_state = PXFIFO_GET_DATA_LOW;
    }
    break;

  case PXFIFO_GET_DATA_LOW:
    if (handle->ctx.tick_count & 1)
    {
      status = fetch_tile_data(&fetcher_ctx, 0);
      handle->fifo_state = PXFIFO_GET_DATA_HIGH;
    }
    break;

  case PXFIFO_GET_DATA_HIGH:
    if (handle->ctx.tick_count & 1)
    {
      status = fetch_tile_data(&fetcher_ctx, 1);
      handle->fifo_state = PXFIFO_SLEEP;
    }
    break;

  case PXFIFO_SLEEP:
    if (handle->ctx.tick_count & 1)
    {
      handle->fifo_state = PXFIFO_PUSH;
    }
    break;

  case PXFIFO_PUSH:
    status = handle_pxfifo_push_data(handle);
    break;

  default:
    break;
  }

  RETURN_STATUS_IF_NOT_OK(status);

  handle->ctx.tick_count++;
  return STATUS_OK;
}

static status_code_t pxfifo_shift_out(pxfifo_handle_t *const handle, pixel_data_t *const pixel_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(pixel_out);

  pxfifo_item_t fifo_item;
  status_code_t status = STATUS_OK;

  pixel_out->data_valid = 0;

  if (ring_buffer_size(&handle->bg_fifo.fifo_buffer) <= 8)
  {
    return STATUS_OK;
  }

  if (!ring_buffer_read(&handle->bg_fifo.fifo_buffer, &fifo_item))
  {
    return STATUS_ERR_GENERIC;
  }

  if (handle->ctx.line_x >= (handle->lcd_handle->registers.scroll_x % 8))
  {
    status = lcd_get_palette_color(handle->lcd_handle, fifo_item.palette, fifo_item.pixel_color, &pixel_out->color);
    RETURN_STATUS_IF_NOT_OK(status);

    pixel_out->screen_x = handle->ctx.pushed_x;
    pixel_out->screen_y = handle->lcd_handle->registers.ly;
    pixel_out->data_valid = 1;

    handle->ctx.pushed_x++;
  }

  handle->ctx.line_x++;

  return STATUS_OK;
}

static status_code_t pxfifo_mix_sprite_pixel(pxfifo_handle_t *const handle, pxfifo_item_t *const bgw_pixel)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bgw_pixel);

  lcd_handle_t *const lcd_handle = handle->lcd_handle;
  pixel_fetcher_state_t *const fetcher_state = &handle->pixel_fetcher;

  for (uint8_t i = 0; i < fetcher_state->fetched_sprite_count; i++)
  {
    fetched_sprite_tile_t *const current_sprite = &fetcher_state->sprite_tile_data[i];

    int16_t sprite_x = (current_sprite->tile_entry.x_pos - 8) + (lcd_handle->registers.scroll_x % 8);

    if (sprite_x + 8 < handle->ctx.fifo_x)
    {
      continue;
    }

    int16_t offset = handle->ctx.fifo_x - sprite_x;

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

static status_code_t handle_pxfifo_push_data(pxfifo_handle_t *const handle)
{
  if (ring_buffer_size(&handle->bg_fifo.fifo_buffer) > 8)
  {
    return STATUS_OK;
  }

  status_code_t status = STATUS_OK;
  int16_t x = (handle->pixel_fetcher.fetcher_x_index * 8) - (8 - (handle->lcd_handle->registers.scroll_x % 8));

  for (int8_t i = 0; i < PIXELS_PER_TILE; i++)
  {
    pxfifo_item_t fifo_item = {
        .pixel_color = bgw_pixel_color_index(&handle->pixel_fetcher.bgw_tile_data, i),
        .obj_priority = 0,
        .bg_priority = 0,
        .palette = PALETTE_BGW,
    };

    if (!(handle->lcd_handle->registers.lcd_ctrl & LCD_CTRL_BGW_EN))
    {
      fifo_item.pixel_color = 0;
    }

    if (handle->lcd_handle->registers.lcd_ctrl & LCD_CTRL_OBJ_EN)
    {
      // Pre-mix background / window pixel with sprite pixels to eliminate the need to use 2 FIFOs
      status = pxfifo_mix_sprite_pixel(handle, &fifo_item);
      RETURN_STATUS_IF_NOT_OK(status);
    }

    if (x >= 0)
    {
      ring_buffer_write(&handle->bg_fifo.fifo_buffer, &fifo_item);
      handle->ctx.fifo_x++;
    }
  }

  handle->ctx.tick_count = 0;
  handle->fifo_state = PXFIFO_GET_TILE_NUM;

  return STATUS_OK;
}
