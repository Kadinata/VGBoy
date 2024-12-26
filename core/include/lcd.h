#ifndef __DMG_LCD_H__
#define __DMG_LCD_H__

#include <stdint.h>

#include "bus_interface.h"
#include "status_code.h"

typedef union
{
  struct
  {
    uint8_t lcd_ctrl;      /* 0xFF40: LCDC: LCD Control */
    uint8_t lcd_stat;      /* 0xFF41: STAT: LCD Status */
    uint8_t scroll_y;      /* 0xFF42: BG viewport Y position */
    uint8_t scroll_x;      /* 0xFF43: BG viewport X position */
    uint8_t ly;            /* 0xFF44: LY: LCD Y (Read only) */
    uint8_t ly_comp;       /* 0xFF45: LYC: LY Compare */
    uint8_t _unused;       /* 0xFF46: DMA: DMA address (handled in different module) */
    uint8_t bg_palette;    /* 0xFF47: BGP: BG palette data (Non-CGB mode only) */
    uint8_t obj_pallete_0; /* 0xFF48: OBP 0: OBJ palette 0 data */
    uint8_t obj_pallete_1; /* 0xFF49: OBP 1: OBJ palette 1 data */
    uint8_t window_y;      /* 0xFF4A: WY: Window Y pos */
    uint8_t window_x;      /* 0xFF4B: WX: Window X pos + 7 */
  };
  uint8_t buffer[12];
} lcd_registers_t;

typedef struct
{
  lcd_registers_t registers;
  bus_interface_t bus_interface;

  uint32_t bg_colors[4];
  uint32_t sp1_colors[4];
  uint32_t sp2_colors[4];
} lcd_handle_t;

typedef enum
{
  MODE_HBLANK,
  MODE_VBLANK,
  MODE_OAM,
  MODE_XFER,
} lcd_mode_t;

status_code_t lcd_init(lcd_handle_t *const handle);

#endif /* __DMG_LCD_H__ */
