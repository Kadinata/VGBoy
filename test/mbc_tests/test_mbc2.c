#include "unity.h"

#include <string.h>

#include "mbc.h"
#include "rom.h"
#include "bus_interface.h"

#include "mbc_test_helper.h"
#include "mock_rtc.h"

TEST_FILE("mbc.c")

static mbc_handle_t mbc;
static uint8_t *rom_data;

#define TEST_MBC_2_READ_FROM_ROM_BANK(BANK_NUM)                                                 \
  void test_mbc2__can_read_bank_##BANK_NUM(void)                                                \
  {                                                                                             \
    uint8_t data;                                                                               \
    switch_rom_bank(&mbc, BANK_NUM);                                                            \
    for (uint16_t address = 0x4000; address < 0x8000; address++)                                \
    {                                                                                           \
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data)); \
      TEST_ASSERT_EQUAL_HEX8(0xB0 | BANK_NUM, data);                                            \
    }                                                                                           \
  }

#define TEST_MBC_2_IGNORES_WRITES_TO_ROM_BANK(BANK_NUM)                                         \
  void test_mbc2__ignores_writes_to_bank_##BANK_NUM(void)                                       \
  {                                                                                             \
    uint8_t data;                                                                               \
    switch_rom_bank(&mbc, BANK_NUM);                                                            \
                                                                                                \
    for (uint16_t address = 0x4000; address < 0x8000; address++)                                \
    {                                                                                           \
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55)); \
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data)); \
      TEST_ASSERT_EQUAL_HEX8(0xB0 | BANK_NUM, data);                                            \
    }                                                                                           \
  }

void switch_rom_bank(mbc_handle_t *const mbc, uint8_t const bank_num)
{
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x0100, bank_num));
  TEST_ASSERT_EQUAL_INT(bank_num, mbc->rom.active_bank_num);
}

void setUp(void)
{
  memset(&mbc, 0, sizeof(mbc_handle_t));
  mbc_init(&mbc);

  rom_data = create_rom(ROM_MBC2, 0x01, MBC_EXT_RAM_SIZE_8K);
  TEST_ASSERT_NOT_NULL(rom_data);

  rtc_is_present_ExpectAndReturn(&mbc.rtc, false);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_load_rom(&mbc, rom_data, 0x10000));
}

void tearDown(void)
{
  mbc_cleanup(&mbc);
}

void test_mbc2__load_rom_sets_up_rom_banks(void)
{
  TEST_ASSERT_EQUAL_INT(4, mbc.rom.num_banks);
  TEST_ASSERT_EQUAL_INT(1, mbc.rom.active_bank_num);
}

void test_mbc2__load_rom_allocates_ram(void)
{
  TEST_ASSERT_EQUAL_INT(1, mbc.ext_ram.num_banks);
  TEST_ASSERT_NOT_NULL(mbc.ext_ram.data);
}

void test_mbc2__load_rom_does_not_initialize_battery(void)
{
  TEST_ASSERT_FALSE(mbc.batt.present);
}

void test_mbc2__load_rom_does_not_initialize_rtc(void)
{
  TEST_ASSERT_FALSE(mbc.rtc.state.present);
}

void test_mbc2__can_read_bank_0(void)
{
  uint8_t data;

  for (uint16_t address = 0x150; address < 0x4000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xB0, data);
  }
}

TEST_MBC_2_READ_FROM_ROM_BANK(1);
TEST_MBC_2_READ_FROM_ROM_BANK(2);
TEST_MBC_2_READ_FROM_ROM_BANK(3);

TEST_MBC_2_IGNORES_WRITES_TO_ROM_BANK(1);
TEST_MBC_2_IGNORES_WRITES_TO_ROM_BANK(2);
TEST_MBC_2_IGNORES_WRITES_TO_ROM_BANK(3);

void test_mbc2__ignores_reads_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xA1FF; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc2__ignores_writes_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xA1FF; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc2__can_read_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0200, 0x0A));

  for (uint16_t address = 0xA000; address < 0xA1FF; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x00, data);
  }
}

void test_mbc2__can_write_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0200, 0x0A));

  for (uint16_t address = 0xA000; address < 0xA1FF; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x55, data);
  }
}

void test_mbc2__returns_error_when_reading_beyond_bank_1(void)
{
  uint8_t data;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc.bus_interface, 0x8000, &data));
}

void test_mbc2__returns_error_when_writing_beyond_bank_1(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc.bus_interface, 0x8000, 0x55));
}

void test_mbc2__returns_error_when_reading_outside_ram_address_range(void)
{
  stub_test_ram_returns_error_when_reading_outside_address_range(&mbc);
}

void test_mbc2__returns_error_when_writing_outside_ram_address_range(void)
{
  stub_test_ram_returns_error_when_writing_outside_address_range(&mbc);
}