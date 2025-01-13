#include "lcd.h"

#include <stdint.h>

#include "bus_interface.h"
#include "color.h"
#include "status_code.h"

static status_code_t lcd_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t lcd_write(void *const resource, uint16_t const address, uint8_t const data);

static const color_rgba_t default_palette_colors[4] = {
    {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF},
    {.r = 0xAA, .g = 0xAA, .b = 0xAA, .a = 0xFF},
    {.r = 0x55, .g = 0x55, .b = 0x55, .a = 0xFF},
    {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF},
};

status_code_t lcd_init(lcd_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  handle->registers.lcd_ctrl = 0x91;
  handle->registers.lcd_stat = 0x02;
  handle->registers.scroll_x = 0x00;
  handle->registers.scroll_y = 0x00;
  handle->registers.bg_palette = 0xFC;
  handle->registers.obj_palette_0 = 0xFF;
  handle->registers.obj_palette_1 = 0xFF;
  handle->registers.ly_comp = 0x00;
  handle->registers.ly = 0x00;
  handle->registers.window_x = 0x00;
  handle->registers.window_y = 0x00;

  return bus_interface_init(&handle->bus_interface, lcd_read, lcd_write, handle);
}

status_code_t lcd_get_palette_color(lcd_handle_t *const handle, palette_type_t const palette_type, uint8_t const color_index, color_rgba_t *const color)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(color);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(color_index > 0x3, STATUS_ERR_INVALID_ARG);

  uint8_t palette;

  switch (palette_type)
  {
  case PALETTE_BGW:
    palette = handle->registers.bg_palette;
    break;
  case PALETTE_OBJ_0:
    palette = handle->registers.obj_palette_0;
    break;
  case PALETTE_OBJ_1:
    palette = handle->registers.obj_palette_1;
    break;
  default:
    return STATUS_ERR_INVALID_ARG;
  }

  color->as_hex = default_palette_colors[(palette >> (color_index * 2)) & 0x3].as_hex;

  return STATUS_OK;
}

uint16_t lcd_ctrl_bg_tile_map_address(lcd_handle_t *const handle)
{
  if (handle == NULL)
  {
    return 0;
  }
  return (handle->registers.lcd_ctrl & LCD_CTRL_BG_TILE_MAP) ? 0x9C00 : 0x9800;
}

uint16_t lcd_ctrl_bgw_tile_data_address(lcd_handle_t *const handle)
{
  if (handle == NULL)
  {
    return 0;
  }
  return (handle->registers.lcd_ctrl & LCD_CTRL_BGW_TILE_DATA) ? 0x8000 : 0x8800;
}

uint16_t lcd_ctrl_window_tile_map_address(lcd_handle_t *const handle)
{
  if (handle == NULL)
  {
    return 0;
  }
  return (handle->registers.lcd_ctrl & LCD_CTRL_WINDOW_TILE_MAP) ? 0x9C00 : 0x9800;
}

uint8_t lcd_ctrl_obj_size(lcd_handle_t *const handle)
{
  if (handle == NULL)
  {
    return 0;
  }
  return (handle->registers.lcd_ctrl & LCD_CTRL_OBJ_SIZE) ? 16 : 8;
}

uint8_t lcd_window_enabled(lcd_handle_t *const handle)
{
  return (handle && (handle->registers.lcd_ctrl & LCD_CTRL_WINDOW_EN));
}

uint8_t lcd_window_visible(lcd_handle_t *const handle)
{
  return lcd_window_enabled(handle) &&
         (handle->registers.window_x >= 0) && (handle->registers.window_x <= (SCREEN_WIDTH + 6)) &&
         (handle->registers.window_y >= 0) && (handle->registers.window_y <= SCREEN_HEIGHT);
}

static status_code_t lcd_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  lcd_handle_t *const handle = (lcd_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address > 0xB, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = handle->registers.buffer[address];

  return STATUS_OK;
}

static status_code_t lcd_write(void *const resource, uint16_t const address, uint8_t const data)
{
  lcd_handle_t *const handle = (lcd_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address > 0xB, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  if (address == 0x0001) /* Lower 3 bits of LCD stat are read only */
  {
    handle->registers.buffer[address] = (handle->registers.buffer[address] & 0x07) | (data & ~0x07);
  }
  else if (address != 0x0004) /* LY is read only */
  {
    handle->registers.buffer[address] = data;
  }

  return STATUS_OK;
}
