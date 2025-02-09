#include "mbc_test_helper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_ROM_SIZE (0x40000) // 256 KiB
#define ROM_BANK_SIZE (0x4000)

static uint8_t rom_data[MAX_ROM_SIZE];

static void add_header_checksum(rom_header_t *const header);
static void fill_rom_data(uint8_t *const rom_data, size_t const rom_size);

void *create_rom(cartridge_type_t const cartridge_type, uint8_t const rom_size, uint8_t const ram_size)
{
  memset(rom_data, 0, MAX_ROM_SIZE);

  size_t requested_size = (0x8000 * (1 << rom_size));

  if (requested_size > MAX_ROM_SIZE)
  {
    return NULL;
  }

  uint8_t *start_alloc_region = &(rom_data[MAX_ROM_SIZE - requested_size]);
  rom_header_t *header = (rom_header_t *)&start_alloc_region[0x100];

  snprintf(header->title, 16, "%s", "TEST ROM");
  header->cartridge_type = cartridge_type;
  header->rom_size = rom_size;
  header->ram_size = ram_size;

  add_header_checksum(header);
  fill_rom_data(start_alloc_region, requested_size);

  return start_alloc_region;
}

static void add_header_checksum(rom_header_t *const header)
{
  uint8_t checksum = 0;

  uint8_t *header_bytes = (uint8_t *)header;

  for (uint8_t offset = 0x34; offset <= 0x4C; offset++)
  {
    checksum = checksum - header_bytes[offset] - 1;
  }

  header->header_checksum = checksum;
}

static void fill_rom_data(uint8_t *const rom_data, size_t const rom_size)
{
  for (size_t address = 0x150; address < rom_size; address++)
  {
    rom_data[address] = 0xB0 | (address / ROM_BANK_SIZE);
  }
}
