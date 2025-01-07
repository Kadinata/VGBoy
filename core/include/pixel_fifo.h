#ifndef __DMG_PIXEL_FIFO_H__
#define __DMG_PIXEL_FIFO_H__

#include <stdint.h>

#include "bus_interface.h"
#include "color.h"
#include "lcd.h"
#include "pixel_fetcher.h"
#include "ring_buf.h"
#include "status_code.h"

typedef enum
{
  PXFIFO_GET_TILE_NUM,
  PXFIFO_GET_DATA_LOW,
  PXFIFO_GET_DATA_HIGH,
  PXFIFO_SLEEP,
  PXFIFO_PUSH,
} pxfifo_state_t;

/** TODO: compact */
typedef struct
{
  uint8_t pixel_color;    // Color value between 0 and 3
  palette_type_t palette; // A value between 0 and 7 on CGB; applies only to object on DMG
  uint8_t obj_priority;   // On CGB this is the OAM index for the object; on DMG this doesn’t exist
  uint8_t bg_priority;    // OBJ-to-BG priority bit;
} pxfifo_item_t;

typedef struct
{
  color_rgba_t color;
  uint16_t screen_x;
  uint16_t screen_y;
  uint8_t data_valid;
} pixel_data_t;

typedef struct
{
  ring_buffer_t fifo_buffer;
  pxfifo_item_t fifo_storage[16];
} pxfifo_t;

typedef struct
{
  uint8_t line_x;
  uint8_t pushed_x;
  uint8_t tick_count;
  uint8_t fifo_x;
} pxfifo_context_t;

typedef struct
{
  pxfifo_state_t fifo_state;
  pxfifo_t bg_fifo;
  pxfifo_context_t ctx;
  bus_interface_t bus_interface;
  lcd_handle_t *lcd_handle;
  pixel_fetcher_state_t pixel_fetcher;
} pxfifo_handle_t;

typedef struct
{
  bus_interface_t *bus_interface;
  lcd_handle_t *lcd_handle;
} pxfifo_init_param_t;

status_code_t pxfifo_init(pxfifo_handle_t *const handle, pxfifo_init_param_t *const param);
status_code_t pxfifo_reset(pxfifo_handle_t *const handle);
status_code_t pxfifo_shift_pixel(pxfifo_handle_t *const handle, pixel_data_t *const pixel_out);

#endif /* __DMG_PIXEL_FIFO_H__ */
