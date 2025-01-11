#ifndef __DMG_PIXEL_FIFO_H__
#define __DMG_PIXEL_FIFO_H__

#include <stdint.h>

#include "bus_interface.h"
#include "color.h"
#include "lcd.h"
#include "pixel_fetcher.h"
#include "ring_buf.h"
#include "status_code.h"

/**
 * Pixel FIFO FSM state definitions
 */
typedef enum
{
  PXFIFO_GET_TILE_NUM,
  PXFIFO_GET_DATA_LOW,
  PXFIFO_GET_DATA_HIGH,
  PXFIFO_SLEEP,
  PXFIFO_PUSH,
} pxfifo_state_t;

/**
 * Definition of each item on the FIFO
 * TODO: compact
 */
typedef struct
{
  uint8_t pixel_color;    /** Pixel color value between 0 and 3 */
  palette_type_t palette; /** A value between 0 and 7 on CGB; applies only to object on DMG */
  uint8_t obj_priority;   /** On CGB this is the OAM index for the object; on DMG this doesn’t exist */
  uint8_t bg_priority;    /** OBJ-to-BG priority bit */
} pxfifo_item_t;

/**
 * Ring buffer as the underlying FIFO implementation
 */
typedef struct
{
  ring_buffer_t buffer;      /** Ring buffer internal state */
  pxfifo_item_t storage[16]; /** Ring buffer underlying data storage */
} pxfifo_buffer_t;

/**
 * Data for pixels that are ready to be rendered on the display
 */
typedef struct
{
  color_rgba_t color; /** Actual RGBA color of the pixel to be rendered */
  uint16_t screen_x;  /** X coordinate of the pixel on the screen */
  uint16_t screen_y;  /** Y coordinate of the pixel on the screen */
  uint8_t data_valid; /** Indicates if this pixel holds valid data. If set to 0, this pixel should not be rendered */
} pixel_data_t;

/**
 * FIFO internal counters
 */
typedef struct
{
  uint8_t pushed_px; /** The number of pixels that have been pushed onto the FIFO on the current scanline */
  uint8_t popped_px; /** The number of pixels that have been popped off the FIFO on the current scanline */
  uint8_t render_px; /** The number of pixels that have been rendered on the screen on the current scanline */
  uint8_t ticks;     /** Keeps track of the number of ticks that has elapsed */
} pxfifo_counter_t;

/**
 * Top-level pixel FIFO object definition
 */
typedef struct
{
  pxfifo_state_t fifo_state;           /** Current FSM state of the FIFO */
  pxfifo_buffer_t bg_fifo;             /** Buffer to hold background and window tile pixels */
  pxfifo_counter_t counters;           /** FIFO internal counters */
  bus_interface_t bus_interface;       /** Bus interface to allow the FIFO to read from VRAM */
  lcd_handle_t *lcd;                   /** LCD registers handle */
  pixel_fetcher_state_t pixel_fetcher; /** Pixel fetcher object */
} pxfifo_handle_t;

/**
 * Initialization parameters definition
 */
typedef struct
{
  bus_interface_t *bus_interface; /** Pinter to bus interface to allow the FIFO to read from VRAM */
  lcd_handle_t *lcd;              /** Pointerr to an LCD registers handle */
} pxfifo_init_param_t;

/**
 * Initializes a pixel FIFO
 *
 * @param pxfifo Pointer to a pixel FIFO to initialize
 * @param param Pointer to an initialization parameter object
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t pxfifo_init(pxfifo_handle_t *const pxfifo, pxfifo_init_param_t *const param);

/**
 * Reset the pixel FIFO. This includes resetting the pixel fetcher, internal counters,
 * and the FIFO's FSM state.
 *
 * @param pxfifo Pointer to a pixel FIFO to reset
 *
 * @return `STATUS_OK` if successful, otherwise appropriate error code.
 */
status_code_t pxfifo_reset(pxfifo_handle_t *const pxfifo);

/**
 * Fetch tiles, pushed the tiles' pixels onto the FIFO, and subsequently pop off a pixel from
 * the FIFO.
 *
 * @param pxfifo Pointer to a pixel FIFO to run the pixel processing
 * @param pixel_out Pointer to a pixel data to store the popped-off pixel
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t pxfifo_shift_pixel(pxfifo_handle_t *const pxfifo, pixel_data_t *const pixel_out);

#endif /* __DMG_PIXEL_FIFO_H__ */
