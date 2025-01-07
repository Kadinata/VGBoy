#ifndef __DMG_PPU_H__
#define __DMG_PPU_H__

#include <stdint.h>

#include "bus_interface.h"
#include "interrupt.h"
#include "lcd.h"
#include "oam.h"
#include "pixel_fifo.h"
#include "status_code.h"

typedef status_code_t (*fps_sync_callback_fn)(void *const ctx);

typedef struct
{
  fps_sync_callback_fn callback;
  void *ctx;
} fps_sync_handler_t;

typedef struct
{
  lcd_handle_t lcd;
  oam_handle_t oam;
  interrupt_handle_t *interrupt;
  pxfifo_handle_t pxfifo;
  fps_sync_handler_t fps_sync_handler;
  uint32_t current_frame;
  uint32_t line_ticks;
  uint32_t video_buffer[SCREEN_HEIGHT * SCREEN_WIDTH];
} ppu_handle_t;

typedef struct
{
  bus_interface_t *bus_interface;
  interrupt_handle_t *interrupt;
} ppu_init_param_t;

status_code_t ppu_init(ppu_handle_t *const ppu, ppu_init_param_t *const param);
status_code_t ppu_tick(ppu_handle_t *const ppu);
status_code_t ppu_register_fps_sync_handler(ppu_handle_t *const ppu, fps_sync_handler_t fps_sync_handler);

#endif /* __DMG_PPU_H__ */
