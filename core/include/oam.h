#ifndef __DMG_OAM_H__
#define __DMG_OAM_H__

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

#define OAM_ENTRY_SIZE (40)
#define MAX_SPRITES_PER_LINE (10)

/**
 * OAM attribute bit field
 */
typedef enum
{
  OAM_ATTR_CGB_PALETTE_NUM = (0x7),    /** (CGB only) Indicates which of the OBP0-7 palette to use */
  OAM_ATTR_CGB_VRAM_BANK = (1 << 3),   /** (CGB only) Indicates which VRAM bank (0 or 1) to fetch from */
  OAM_ATTR_DMG_PALETTE_NUM = (1 << 4), /** (Non-CGB only) Indicates which of the OBP0-1 palette to use */
  OAM_ATTR_X_FLIP = (1 << 5),          /** Indicates whether the sprite should be flipped horizontally */
  OAM_ATTR_Y_FLIP = (1 << 6),          /** Indicates whether the sprite should be flipped vertically */
  OAM_ATTR_BG_PRIORITY = (1 << 7),     /** Indicates whether the background or window should be drawn over the sprite instead */
} oam_attr_t;

typedef enum
{
  OBJ_SIZE_SMALL = 8,
  OBJ_SIZE_LARGE = 16,
} obj_size_t;

/**
 * OAM entry definition
 */
typedef struct
{
  uint8_t y_pos; /** Object's (sprite's) vertical position on the screen + 16 */
  uint8_t x_pos; /** Object's (sprite's) horizontal position on the screen + 8 */
  uint8_t tile;  /** Object's (sprite's) tile index in VRAM */
  uint8_t attrs; /** Object's (sprite's) attribute flags */
} oam_entry_t;

/**
 * Structure to store sprites collected during an OAM scan
 */
typedef struct
{
  uint8_t sprite_count;                                /** Number of collected sprites after the scan */
  oam_entry_t sprite_attributes[MAX_SPRITES_PER_LINE]; /** Copies of collected sprite data */
} oam_scanned_sprites_t;

/**
 * Top-level OAM data structure definition
 */
typedef struct
{
  union
  {
    oam_entry_t entries[OAM_ENTRY_SIZE];                   /** OAM entry data store */
    uint8_t oam_buf[sizeof(oam_entry_t) * OAM_ENTRY_SIZE]; /** OAM entry data store as byte array */
  };
  bus_interface_t bus_interface; /** Interface to allow the data bus to write to and read from the OAM */
} oam_handle_t;

/**
 * Initializes the OAM and configures its bus interface
 *
 * @param oam_handle Pointer to the OAM object to initialize
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t oam_init(oam_handle_t *const oam_handle);

/**
 * Scan the OAM for up to sprites that intersects the provided scan line.
 * This function is to be called during mode 2 (OAM scan) of PPU rendering.
 * Sprites returned by this function are sorted by their X coordinate in ascending order.
 *
 * @param oam_handle Pointer to the OAM object to scan the sprites from
 * @param line_y Current scanline y-coordinate, the same as the current value of the LCD register LY
 * @param sprite_size Indicates whether the sprite is 8 pixels or 16 pixels tall, as determined by bit-2 of the LCDC register
 * @param scan_results Pointer to a scan result object to store the scanned sprites.
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t oam_scan(oam_handle_t *const oam_handle, uint8_t const line_y, obj_size_t const sprite_size, oam_scanned_sprites_t *const scan_results);

#endif /* __DMG_OAM_H__ */
