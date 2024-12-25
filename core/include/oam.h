#ifndef __DMG_OAM_H__
#define __DMG_OAM_H__

#include <stdint.h>
#include "status_code.h"

#define OAM_ATTR_CGB_PALETTE_NUM (0x7)
#define OAM_ATTR_CGB_VRAM_BANK (1 << 3)
#define OAM_ATTR_DMG_PALETTE_NUM (1 << 4)
#define OAM_ATTR_X_FLIP (1 << 5)
#define OAM_ATTR_Y_FLIP (1 << 6)
#define OAM_ATTR_BG_PRIORITY (1 << 7)

typedef struct
{
  uint8_t y_pos;
  uint8_t x_pos;
  uint8_t tile;
  uint8_t attrs;
} oam_entry_t;

typedef struct
{
  uint16_t offset;
  union
  {
    oam_entry_t entries[40];
    uint8_t oam_buf[sizeof(oam_entry_t) * 40];
  };
} oam_handle_t;

status_code_t oam_read(oam_handle_t *const oam_handle, uint16_t const address, uint8_t *const data);
status_code_t oam_write(oam_handle_t *const oam_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_OAM_H__ */
