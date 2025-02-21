#include "mbc_test_helper.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "bus_interface.h"
#include "mbc.h"
#include "rom.h"

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

void stub_write_then_read_address_range(mbc_handle_t *const mbc, uint16_t const start_address, uint16_t const range, uint8_t const write_data, uint8_t const expected_data)
{
  uint8_t data;

  for (uint16_t address = start_address; address < (start_address + range); address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, address, write_data));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc->bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(expected_data, data);
  }
}

void stub_read_address_range(mbc_handle_t *const mbc, uint16_t const start_address, uint16_t const range, uint8_t const expected_data)
{
  uint8_t data;

  for (uint16_t address = start_address; address < (start_address + range); address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc->bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(expected_data, data);
  }
}

void stub_test_ram_returns_error_when_reading_outside_address_range(mbc_handle_t *const mbc)
{
  uint8_t data;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc->bus_interface, 0x9FFF, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc->bus_interface, 0xC000, &data));
}

void stub_test_ram_returns_error_when_writing_outside_address_range(mbc_handle_t *const mbc)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc->bus_interface, 0x9FFF, 0x55));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc->bus_interface, 0xC000, 0x55));
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
