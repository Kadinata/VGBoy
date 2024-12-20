#ifndef __DMG_RAM_H__
#define __DMG_RAM_H__

#include <stdint.h>
#include "status_code.h"

#define VRAM_SIZE (0x2000)
#define WRAM_SIZE (0x2000)
#define HRAM_SIZE (0x7F)


typedef struct {
  uint16_t offset;
  uint8_t buf[WRAM_SIZE];
} wram_t;

typedef struct {
  uint16_t offset;
  uint8_t buf[VRAM_SIZE];
} vram_t;

typedef struct {
  uint16_t offset;
  uint8_t buf[HRAM_SIZE];
} hram_t;

typedef struct
{
  wram_t wram;
  vram_t vram;
  hram_t hram;
} ram_handle_t;

status_code_t ram_read(ram_handle_t *const ram_handle, uint16_t const address, uint8_t *const data);
status_code_t ram_write(ram_handle_t *const ram_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_RAM_H__ */
