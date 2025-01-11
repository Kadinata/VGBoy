#ifndef __DMG_MBC_H__
#define __DMG_MBC_H__

#include <stdint.h>

#include "bus_interface.h"
#include "rom.h"
#include "status_code.h"

#define MAX_RAM_BANKS (16)

typedef enum
{
  MBC_EXT_RAM_SIZE_NO_RAM = 0,
  MBC_EXT_RAM_SIZE_UNUSED = 1,
  MBC_EXT_RAM_SIZE_8K = 2,
  MBC_EXT_RAM_SIZE_32K = 3,
  MBC_EXT_RAM_SIZE_128K = 4,
  MBC_EXT_RAM_SIZE_64K = 5,
} mbc_ext_ram_size_t;

typedef enum
{
  BANK_MODE_SIMPLE = 0,
  BANK_MODE_ADVANCED = 1,
} banking_mode_t;

typedef struct
{
  uint8_t active_bank_num;
  uint8_t *active_switchable_bank;
  rom_handle_t content;
  bus_interface_t bus_interface;
} mbc_rom_t;

typedef struct
{
  uint8_t has_battery;
  uint8_t enabled;
  uint8_t num_banks;
  uint8_t *banks[16];
  uint8_t *active_bank;
  bus_interface_t bus_interface;
} mbc_ext_ram_t;

typedef struct
{
  mbc_rom_t rom;
  mbc_ext_ram_t ext_ram;
  banking_mode_t banking_mode;
  bus_interface_t bus_interface;
} mbc_handle_t;

status_code_t mbc_init(mbc_handle_t *const mbc);
status_code_t mbc_load_rom(mbc_handle_t *const mbc, const char *file);
status_code_t mbc_cleanup(mbc_handle_t *const mbc);

#endif /* __DMG_MBC_H__ */
