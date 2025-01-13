#ifndef __DMG_ROM_H__
#define __DMG_ROM_H__

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

/**
 * Identifier for the types of hardware available
 * on the cartridge. For more details, see:
 * https://gbdev.io/pandocs/The_Cartridge_Header.html#0147--cartridge-type
 *
 * Note: using the packed attribute to ensure these values are 8-bit long
 */
typedef enum __attribute__((packed))
{
  ROM_ONLY = 0x00,
  ROM_MBC1 = 0x01,
  ROM_MBC1_RAM = 0x02,
  ROM_MBC1_RAM_BATT = 0x03,
  ROM_MBC2 = 0x05,
  ROM_MBC2_BATT = 0x06,
  ROM_RAM = 0x08,
  ROM_RAM_BATT = 0x09,
  ROM_MMM01 = 0x0B,
  ROM_MMM01_RAM = 0x0C,
  ROM_MMM01_RAM_BATT = 0x0D,
  ROM_MBC3_TIMER_BATT = 0x0F,
  ROM_MBC3_TIMER_RAM_BATT = 0x10,
  ROM_MBC3 = 0x11,
  ROM_MBC3_RAM = 0x12,
  ROM_MBC3_RAM_BATT = 0x13,
  ROM_MBC5 = 0x19,
  ROM_MBC5_RAM = 0x1A,
  ROM_MBC5_RAM_BATT = 0x1B,
  ROM_MBC5_RUMBLE = 0x1C,
  ROM_MBC5_RUMBLE_RAM = 0x1D,
  ROM_MBC5_RUMBLE_RAM_BATT = 0x1E,
  ROM_MBC6 = 0x20,
  ROM_MBC7_SENSOR_RUMBLE_RAM_BATT = 0x22,
  ROM_POCKET_CAMERA = 0xFC,
  ROM_BANDAI_TAMA5 = 0xFD,
  ROM_HUC3 = 0xFE,
  ROM_HUC1_RAM_BATT = 0xFF,
} cartridge_type_t;

/**
 * Fields of the 80-bytes long ROM header located at 0x100 - 0x14F
 * For more information, see:
 * https://gbdev.io/pandocs/The_Cartridge_Header.html#the-cartridge-header
 */
typedef struct
{
  uint8_t entry_point[4];          // 0x100 - 0x103: Instruction entry point
  uint8_t logo[48];                // 0x104 - 0x133: Logo
  char title[16];                  // 0x134 - 0x143: Title
  uint16_t new_license_code;       // 0x144 - 0x145: New license code
  uint8_t sgb_flag;                // 0x146: SGB flag
  cartridge_type_t cartridge_type; // 0x147: Cartridge type
  uint8_t rom_size;                // 0x148: ROM size
  uint8_t ram_size;                // 0x149: RAM size
  uint8_t destination_code;        // 0x14A: Destination code
  uint8_t old_license_code;        // 0x14B: Old license code
  uint8_t mask_rom_version;        // 0x14C: Mask ROM version numberr
  uint8_t header_checksum;         // 0x14D: ROM header checksum
  uint16_t global_checksum;        // 0x14E - 0x14F: ROM global checksum
} rom_header_t;

/** */
typedef struct
{
  rom_header_t *header;
  uint8_t *data;
} rom_handle_t;

/**
 * Open a ROM file and load its contents to the provided ROM handle.
 * This function allocates memory for the ROM contents, and unload_rom
 * must be called eventually to free the allocated resource.
 * @param handle - Pointer to a ROM handle object where the loaded ROM
 *                 contents will be stored
 * @param file - Path to the ROM file
 * @return STATUS_OK if no error, otherwise appropriate error code.
 */
status_code_t rom_load(rom_handle_t *const handle, const char *file);

/**
 * Deallocate resources used for loaded ROM contents.
 * @param handle - Pointer to a ROM handle to free
 * @return STATUS_OK if no error, otherwise appropriate error code.
 */
status_code_t rom_unload(rom_handle_t *const handle);

#endif /* __DMG_ROM_H__ */
