#include "rom.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bus_interface.h"
#include "logging.h"
#include "status_code.h"

#define ROM_HEADER_ADDR (0x100)
#define ROM_SIZE_KB(rom_size) (32 * (1 << rom_size))

static inline status_code_t verify_header_checksum(rom_data_t *const handle);

status_code_t rom_load(rom_data_t *const rom, uint8_t *const rom_data, const size_t size)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rom);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rom_data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(rom->data != NULL, STATUS_ERR_ALREADY_INITIALIZED);

  if (size < 0x14F)
  {
    Log_E("ROM data size is too small! (%zu bytes)", size);
    return STATUS_ERR_INVALID_ARG;
  }

  rom->size = size;
  rom->data = rom_data;
  rom->header = (rom_header_t *)&rom->data[ROM_HEADER_ADDR];

  Log_I("ROM Metadata:");
  Log_I("- Title:        %s", rom->header->title);
  Log_I("- Type:         0x%02X", rom->header->cartridge_type);
  Log_I("- ROM size:     %u KiB", ROM_SIZE_KB(rom->header->rom_size));
  Log_I("- RAM size:     %u", rom->header->ram_size);
  Log_I("- License code: 0x%02X", rom->header->new_license_code);
  Log_I("- ROM version:  0x%02X", rom->header->mask_rom_version);

  return verify_header_checksum(rom);
}

status_code_t rom_unload(rom_data_t *const rom)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rom);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(rom->data, STATUS_ERR_ALREADY_FREED);

  rom->data = NULL;
  rom->header = NULL;
  rom->size = 0;

  Log_I("ROM unloaded successfully");
  return STATUS_OK;
}

static status_code_t verify_header_checksum(rom_data_t *const rom)
{
  uint8_t checksum = 0;

  for (uint16_t address = 0x134; address <= 0x14C; address++)
  {
    checksum = checksum - rom->data[address] - 1;
  }

  Log_I("ROM header checksum: 0x%02X", rom->header->header_checksum);

  if (checksum != rom->header->header_checksum)
  {
    Log_E("ROM header checksum check failed (0x%02X)", checksum);
    return STATUS_ERR_CHECKSUM_FAILURE;
  }

  Log_I("ROM header checksum check passed");
  return STATUS_OK;
}
