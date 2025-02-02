#ifndef __DMG_MBC_H__
#define __DMG_MBC_H__

#include <stdint.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "rom.h"
#include "status_code.h"

#define MAX_RAM_BANKS (16)

typedef status_code_t (*save_game_callback_fn)(uint8_t *const data, size_t const size);

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
  uint16_t num_banks;
  uint8_t *active_switchable_bank;
  uint16_t active_bank_num;
  rom_data_t content;
  bus_interface_t bus_interface;
} mbc_rom_t;

typedef struct
{
  bool enabled;
  uint8_t num_banks;
  uint8_t *banks[16];
  uint8_t *active_bank;
  uint8_t active_bank_num;
  bus_interface_t bus_interface;
} mbc_ext_ram_t;

typedef struct
{
  bool present;
  bool has_unsaved_data;
} mbc_battery_t;

typedef struct
{
  save_game_callback_fn save_game;
  save_game_callback_fn load_game;
} mbc_callbacks_t;

typedef struct
{
  mbc_rom_t rom;
  mbc_ext_ram_t ext_ram;
  mbc_battery_t batt;
  banking_mode_t banking_mode;
  bus_interface_t bus_interface;
  mbc_callbacks_t callbacks;
} mbc_handle_t;

status_code_t mbc_init(mbc_handle_t *const mbc);
status_code_t mbc_load_rom(mbc_handle_t *const mbc, uint8_t *const rom_data, const size_t size);
status_code_t mbc_register_callbacks(mbc_handle_t *const mbc, save_game_callback_fn const save_game, save_game_callback_fn const load_game);
status_code_t mbc_cleanup(mbc_handle_t *const mbc);
status_code_t mbc_save_game(mbc_handle_t *const mbc);
status_code_t mbc_load_saved_game(mbc_handle_t *const mbc);
status_code_t mbc_reload_banks(mbc_handle_t *const mbc);

#endif /* __DMG_MBC_H__ */
