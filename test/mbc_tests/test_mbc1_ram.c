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

void switch_ram_bank(mbc_handle_t *const mbc, uint8_t const bank_num)
{
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x6000, 0x01));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x4000, bank_num));
  TEST_ASSERT_EQUAL_INT(bank_num, mbc->ext_ram.active_bank_num);
}

void setUp(void)
{
  memset(&mbc, 0, sizeof(mbc_handle_t));
  mbc_init(&mbc);

  rom_data = create_rom(ROM_MBC1_RAM, 0x01, MBC_EXT_RAM_SIZE_32K);
  TEST_ASSERT_NOT_NULL(rom_data);

  rtc_is_present_ExpectAndReturn(&mbc.rtc, false);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_load_rom(&mbc, rom_data, 0x10000));
}

void tearDown(void)
{
  mbc_cleanup(&mbc);
}

void test_mbc1_ram__load_rom_allocates_ram(void)
{
  TEST_ASSERT_EQUAL_INT(4, mbc.ext_ram.num_banks);
  TEST_ASSERT_NOT_NULL(mbc.ext_ram.data);
}

void test_mbc1_ram__load_rom_does_not_initialize_battery(void)
{
  TEST_ASSERT_FALSE(mbc.batt.present);
}

void test_mbc1_ram__load_rom_does_not_initialize_rtc(void)
{
  TEST_ASSERT_FALSE(mbc.rtc.state.present);
}

void test_mbc1_ram__ignores_reads_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc1_ram__ignores_writes_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc1_ram__can_read_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x00, data);
  }
}

void test_mbc1_ram__can_write_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x55, data);
  }
}

void test_mbc1_ram__can_switch_ram_banks(void)
{
  uint8_t data;

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  /* Write first */
  for (uint8_t bank_num = 0; bank_num < 4; bank_num++)
  {
    switch_ram_bank(&mbc, bank_num);
    for (uint16_t address = 0xA000; address < 0xC000; address++)
    {
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
      TEST_ASSERT_EQUAL_HEX8(0x00, data);
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0xB0 | bank_num));
    }
  }

  /* Now read */
  for (uint8_t bank_num = 0; bank_num < 4; bank_num++)
  {
    switch_ram_bank(&mbc, bank_num);
    for (uint16_t address = 0xA000; address < 0xC000; address++)
    {
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
      TEST_ASSERT_EQUAL_HEX8(0xB0 | bank_num, data);
    }
  }
}

void test_mbc1_ram__does_not_switch_bank_on_simple_addressing_mode(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x4000, 0x3));
  TEST_ASSERT_EQUAL_INT(0x00, mbc.ext_ram.active_bank_num);
}