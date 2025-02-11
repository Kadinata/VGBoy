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

#define TEST_MBC_3_RTC_READ_FROM_ROM_BANK(BANK_NUM)                                             \
  void test_mbc3_rtc__can_read_bank_##BANK_NUM(void)                                            \
  {                                                                                             \
    uint8_t data;                                                                               \
    switch_rom_bank(&mbc, BANK_NUM);                                                            \
    for (uint16_t address = 0x4000; address < 0x8000; address++)                                \
    {                                                                                           \
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data)); \
      TEST_ASSERT_EQUAL_HEX8(0xB0 | BANK_NUM, data);                                            \
    }                                                                                           \
  }

#define TEST_MBC_3_RTC_IGNORES_WRITES_TO_ROM_BANK(BANK_NUM)                                     \
  void test_mbc3_rtc__ignores_writes_to_bank_##BANK_NUM(void)                                   \
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
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x2000, bank_num));
  TEST_ASSERT_EQUAL_INT(bank_num, mbc->rom.active_bank_num);
}

void switch_ram_bank(mbc_handle_t *const mbc, uint8_t const bank_num)
{
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x4000, bank_num));
  TEST_ASSERT_EQUAL_INT(bank_num, mbc->ext_ram.active_bank_num);
  TEST_ASSERT_FALSE(!!(mbc->flags & MBC_FLAGS_ACCESS_MODE_RTC));
}

void switch_rtc_reg(mbc_handle_t *const mbc, uint8_t const reg_num)
{
  rtc_is_present_ExpectAndReturn(&mbc->rtc, true);
  rtc_select_reg_ExpectAndReturn(&mbc->rtc, reg_num - 0x08, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x4000, reg_num));
  TEST_ASSERT_TRUE(!!(mbc->flags & MBC_FLAGS_ACCESS_MODE_RTC));
}

void setUp(void)
{
  memset(&mbc, 0, sizeof(mbc_handle_t));
  mbc_init(&mbc);

  rom_data = create_rom(ROM_MBC3_TIMER_RAM_BATT, 0x01, MBC_EXT_RAM_SIZE_32K);
  TEST_ASSERT_NOT_NULL(rom_data);

  rtc_init_ExpectAndReturn(&mbc.rtc, STATUS_OK);
  rtc_is_present_ExpectAndReturn(&mbc.rtc, true);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_load_rom(&mbc, rom_data, 0x10000));
}

void tearDown(void)
{
  mbc_cleanup(&mbc);
}

void test_mbc3_rtc__load_rom_sets_up_rom_banks(void)
{
  TEST_ASSERT_EQUAL_INT(4, mbc.rom.num_banks);
  TEST_ASSERT_EQUAL_INT(1, mbc.rom.active_bank_num);
}

void test_mbc3_rtc__load_rom_allocates_ram(void)
{
  TEST_ASSERT_EQUAL_INT(4, mbc.ext_ram.num_banks);
  TEST_ASSERT_NOT_NULL(mbc.ext_ram.data);
}

void test_mbc3_rtc__load_rom_initializes_battery(void)
{
  TEST_ASSERT_TRUE(mbc.batt.present);
}

void test_mbc3_rtc__can_read_bank_0(void)
{
  uint8_t data;

  for (uint16_t address = 0x150; address < 0x4000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xB0, data);
  }
}

TEST_MBC_3_RTC_READ_FROM_ROM_BANK(1);
TEST_MBC_3_RTC_READ_FROM_ROM_BANK(2);
TEST_MBC_3_RTC_READ_FROM_ROM_BANK(3);

// TEST_MBC_3_RTC_IGNORES_WRITES_TO_ROM_BANK(1);
// TEST_MBC_3_RTC_IGNORES_WRITES_TO_ROM_BANK(2);
// TEST_MBC_3_RTC_IGNORES_WRITES_TO_ROM_BANK(3);

void test_mbc3_rtc__ignores_reads_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc3_rtc__ignores_writes_when_ram_is_disabled(void)
{
  uint8_t data;

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0xFF, data);
  }
}

void test_mbc3_rtc__can_read_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  rtc_enable_ExpectAndReturn(&mbc.rtc, true, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x00, data);
  }
}

void test_mbc3_rtc__can_write_when_ram_is_enabled(void)
{
  uint8_t data;

  /* Enable RAM */
  rtc_enable_ExpectAndReturn(&mbc.rtc, true, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0x55));
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    TEST_ASSERT_EQUAL_HEX8(0x55, data);
  }
}

void test_mbc3_rtc__can_switch_ram_banks(void)
{
  uint8_t data;

  /* Enable RAM */
  rtc_enable_ExpectAndReturn(&mbc.rtc, true, STATUS_OK);
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

void test_mbc3_rtc__can_select_rtc_registers(void)
{
  for (uint8_t reg = 0x08; reg <= 0x0C; reg++)
  {
    switch_rtc_reg(&mbc, reg);
  }
}

void test_mbc3_rtc__can_read_from_and_write_to_rtc_registers(void)
{
  uint8_t data;

  /* Enable and write to RAM */
  rtc_enable_ExpectAndReturn(&mbc.rtc, true, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0xA000, 0xB0));

  for (uint8_t reg = 0x08; reg <= 0x0C; reg++)
  {
    switch_rtc_reg(&mbc, reg);

    rtc_read_ExpectAndReturn(&mbc.rtc, NULL, STATUS_OK);
    rtc_read_IgnoreArg_data();
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, 0xA000, &data));

    rtc_write_ExpectAndReturn(&mbc.rtc, 0x55, STATUS_OK);
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0xA000, 0x55));
  }

  /* Switch back to RAM access */
  switch_ram_bank(&mbc, 0);

  /* Verify RAM data was not modified */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, 0xA000, &data));
  TEST_ASSERT_EQUAL_HEX8(0xB0, data);
}

void test_mbc3_rtc__can_latch_rtc_registers(void)
{
  rtc_is_present_ExpectAndReturn(&mbc.rtc, true);
  rtc_latch_ExpectAndReturn(&mbc.rtc, 0x55, STATUS_OK);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x6000, 0x55));
}

void test_mbc3_rtc__returns_error_when_reading_beyond_bank_1(void)
{
  uint8_t data;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc.bus_interface, 0x8000, &data));
}

void test_mbc3_rtc__returns_error_when_writing_beyond_bank_1(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc.bus_interface, 0x8000, 0x55));
}

void test_mbc3_rtc__returns_error_when_reading_outside_ram_address_range(void)
{
  uint8_t data;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc.bus_interface, 0x9FFF, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&mbc.bus_interface, 0xC000, &data));
}

void test_mbc3_rtc__returns_error_when_writing_outside_ram_address_range(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc.bus_interface, 0x9FFF, 0x55));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_write(&mbc.bus_interface, 0xC000, 0x55));
}