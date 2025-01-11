#ifndef __DMG_LCD_H__
#define __DMG_LCD_H__

#include <stdint.h>

#include "bus_interface.h"
#include "color.h"
#include "status_code.h"

#define SCREEN_WIDTH (160)
#define SCREEN_HEIGHT (144)

/**
 * LCD status register bit field definitions
 */
typedef enum
{
  LCD_STAT_PPU_MODE = (0x3),                 /** PPU rendering mode */
  LCD_STAT_LY_EQ_LYC = (1 << 2),             /** Indicates whether the current scanline == LY compare */
  LCD_STAT_HBLANK_MODE_INT_SEL = (1 << 3),   /** If set to 1, trigger an interrupt when H-blank mode is entered */
  LCD_STAT_VBLANK_MODE_INT_SEL = (1 << 4),   /** If set to 1, trigger an interrupt when V-blank mode is entered */
  LCD_STAT_OAM_SCAN_MODE_INT_SEL = (1 << 5), /** If set to 1, trigger an interrupt when OAM scan mode is entered */
  LCD_STAT_LYC_INT_SEL = (1 << 6),           /** If set to 1, trigger an interrupt when LY == LYC */
} lcd_stat_t;

/**
 * LCD control register bit field definitions
 */
typedef enum
{
  LCD_CTRL_BGW_EN = (1 << 0),          /** Indicates whether or not the PPU should render background and window tiles */
  LCD_CTRL_OBJ_EN = (1 << 1),          /** Indicates whether or not the PPU should render sprite / object tiles */
  LCD_CTRL_OBJ_SIZE = (1 << 2),        /** Indicates whether sprites are 8-bit or 16-bit tall */
  LCD_CTRL_BG_TILE_MAP = (1 << 3),     /** If set to 0: tile map starts at 0x9800; If set to 1: 0x9C00 */
  LCD_CTRL_BGW_TILE_DATA = (1 << 4),   /** If set to 0: tile data starts at 0x8800; If set to 1: 0x8000 */
  LCD_CTRL_WINDOW_EN = (1 << 5),       /** Indicates whether or not the PPU should render window tiles */
  LCD_CTRL_WINDOW_TILE_MAP = (1 << 6), /** If set to 0: tile map starts at 0x9800; If set t0 1: 0x9C00 */
  LCD_CTRL_LCD_PPU_EN = (1 << 7),      /** If set to 0, the LCD is turned off */
} lcd_ctrl_t;

/**
 * PPU rendering mode definitions
 * These are the value of LCD stat register bit 0-1
 */
typedef enum
{
  MODE_HBLANK = 0,
  MODE_VBLANK = 1,
  MODE_OAM_SCAN = 2,
  MODE_XFER = 3,
} lcd_mode_t;

/**
 * This is used to determine which palette to use when decoding pixel colors
 */
typedef enum __attribute__((packed))
{
  PALETTE_BGW,
  PALETTE_OBJ_0,
  PALETTE_OBJ_1,
} palette_type_t;

/**
 * LCD registers definitions
 */
typedef union
{
  struct
  {
    uint8_t lcd_ctrl;      /* 0xFF40: LCDC: LCD Control */
    uint8_t lcd_stat;      /* 0xFF41: STAT: LCD Status */
    uint8_t scroll_y;      /* 0xFF42: BG viewport Y position */
    uint8_t scroll_x;      /* 0xFF43: BG viewport X position */
    uint8_t ly;            /* 0xFF44: LY: Current scanline Y coordinate (Read only) */
    uint8_t ly_comp;       /* 0xFF45: LYC: LY Compare */
    uint8_t _unused;       /* 0xFF46: DMA: DMA address (handled in different module) */
    uint8_t bg_palette;    /* 0xFF47: BGP: BG palette data (Non-CGB mode only) */
    uint8_t obj_palette_0; /* 0xFF48: OBP 0: OBJ palette 0 data */
    uint8_t obj_palette_1; /* 0xFF49: OBP 1: OBJ palette 1 data */
    uint8_t window_y;      /* 0xFF4A: WY: Window Y pos */
    uint8_t window_x;      /* 0xFF4B: WX: Window X pos + 7 */
  };
  uint8_t buffer[12]; /* LCD registers as byte array */
} lcd_registers_t;

/**
 * Top-level LCD register object definition
 */
typedef struct
{
  lcd_registers_t registers;     /** LCD registers */
  bus_interface_t bus_interface; /** Interface to allow the data bus to write to and read from LCD registers */
} lcd_handle_t;

/**
 * Initialize the LCD structure
 *
 * @param handle Pointer to an LCD handle object to initialize
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t lcd_init(lcd_handle_t *const handle);

/**
 * Get one of the colors of one of the palettes, as determined by the provided palette type and index.
 * There are 3 palettes: BG palette, OBP-0, and OBP-1. Each palette has 4 colors to choose from, indexed 0-3
 *
 * @param handle Pointer to an LCD handle object storing the palette data
 * @param palette_type Specifies which palette to use (BG palette, OBP-0, or OBP-1)
 * @param color_index Specifies which one of the 4 colors in the palette to choose (0-3)
 * @param color Pointer to a color object to store the result
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t lcd_get_palette_color(lcd_handle_t *const handle, palette_type_t const palette_type, uint8_t const color_index, color_rgba_t *const color);

/**
 * Get the base address of background tile map
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 0x9800 if bit 3 of the LCD control register is not set, 0x9C00 if it's set; 0 if there's an error.
 */
uint16_t lcd_ctrl_bg_tile_map_address(lcd_handle_t *const handle);

/**
 * Get the base address of background / window tile data
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 0x8800 if bit 4 of the LCD control register is not set, 0x8000 if it's set; 0 if there's an error.
 */
uint16_t lcd_ctrl_bgw_tile_data_address(lcd_handle_t *const handle);

/**
 * Get the base address of window tile map
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 0x9800 if bit 6 of the LCD control register is not set, 0x9C00 if it's set; 0 if there's an error.
 */
uint16_t lcd_ctrl_window_tile_map_address(lcd_handle_t *const handle);

/**
 * Determine whether sprites should be 8-bit or 16-bit tall
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 8 if bit 2 of the LCD control register is not set, 16 if it's set; 0 if there's an error.
 */
uint8_t lcd_ctrl_obj_size(lcd_handle_t *const handle);

/**
 * Determine if the window enabled flag is on on the LCD control register
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 1 if the window enabled flag is on, 0 otherwise or if there's an error.
 */
uint8_t lcd_window_enabled(lcd_handle_t *const handle);

/**
 * Determine if the window layer overlaps the screen and should be rendered
 *
 * @param handle Pointer to an LCD handle object
 *
 * @return 1 if the window layer overlaps with the screen, 0 otherwise or if there's an error.
 */
uint8_t lcd_window_visible(lcd_handle_t *const handle);

#endif /* __DMG_LCD_H__ */
