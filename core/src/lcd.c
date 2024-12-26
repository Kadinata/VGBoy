#include "lcd.h"

#include <stdint.h>

#include "bus_interface.h"
#include "logging.h"
#include "status_code.h"

static status_code_t lcd_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t lcd_write(void *const resource, uint16_t const address, uint8_t const data);

status_code_t lcd_init(lcd_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  handle->registers.lcd_ctrl = 0x91;
  handle->registers.scroll_x = 0x00;
  handle->registers.scroll_y = 0x00;
  handle->registers.bg_palette = 0xFC;
  handle->registers.obj_pallete_0 = 0xFF;
  handle->registers.obj_pallete_1 = 0xFF;
  handle->registers.ly_comp = 0x00;
  handle->registers.ly = 0x00;
  handle->registers.window_x = 0x00;
  handle->registers.window_y = 0x00;

  handle->bus_interface.read = lcd_read;
  handle->bus_interface.write = lcd_write;
  handle->bus_interface.resource = handle;

  return STATUS_OK;
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
