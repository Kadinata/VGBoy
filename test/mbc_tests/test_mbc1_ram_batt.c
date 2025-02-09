#include "unity.h"

#include <string.h>
#include <stdbool.h>

#include "mbc.h"
#include "rom.h"
#include "bus_interface.h"

#include "mbc_test_helper.h"
#include "mock_rtc.h"

TEST_FILE("mbc.c")

static mbc_handle_t mbc;
static uint8_t *rom_data;

int load_game_called_num;
int save_game_called_num;
bool cleaned_up;

static status_code_t save_game(saved_game_data_t *const data)
{
  save_game_called_num++;
  return STATUS_OK;
}

static status_code_t load_game(saved_game_data_t *const data)
{
  load_game_called_num++;
  return STATUS_OK;
}

void switch_ram_bank(mbc_handle_t *const mbc, uint8_t const bank_num)
{
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x6000, 0x01));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc->bus_interface, 0x4000, bank_num));
  TEST_ASSERT_EQUAL_INT(bank_num, mbc->ext_ram.active_bank_num);
}

void load_rom(void)
{
  rtc_is_present_ExpectAndReturn(&mbc.rtc, false);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_load_rom(&mbc, rom_data, 0x10000));
}

void register_callbacks(void)
{
  mbc_callbacks_t callbacks = {
      .save_game = save_game,
      .load_game = load_game,
  };

  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_register_callbacks(&mbc, &callbacks));
}

void cleanup(void)
{
  if (cleaned_up)
    return;
  mbc_cleanup(&mbc);
  cleaned_up = true;
}

void setUp(void)
{
  load_game_called_num = 0;
  save_game_called_num = 0;
  cleaned_up = false;

  memset(&mbc, 0, sizeof(mbc_handle_t));
  mbc_init(&mbc);

  rom_data = create_rom(ROM_MBC1_RAM_BATT, 0x01, MBC_EXT_RAM_SIZE_32K);
  TEST_ASSERT_NOT_NULL(rom_data);
}

void tearDown(void)
{
  cleanup();
}

void test_mbc1_ram_batt__load_rom_allocates_ram(void)
{
  load_rom();
  TEST_ASSERT_EQUAL_INT(4, mbc.ext_ram.num_banks);
  TEST_ASSERT_NOT_NULL(mbc.ext_ram.data);
}

void test_mbc1_ram_batt__load_rom_initializes_battery(void)
{
  load_rom();
  TEST_ASSERT_TRUE(mbc.batt.present);
}

void test_mbc1_ram_batt__load_rom_does_not_initialize_rtc(void)
{
  load_rom();
  TEST_ASSERT_FALSE(mbc.rtc.state.present);
}

void test_mbc1_ram_batt__can_register_callbacks(void)
{
  register_callbacks();
}

void test_mbc1_ram_batt__saved_game_is_loaded_when_rom_loads(void)
{
  register_callbacks();

  TEST_ASSERT_EQUAL_INT(0, load_game_called_num);
  load_rom();
  TEST_ASSERT_EQUAL_INT(1, load_game_called_num);
}

void test_mbc1_ram_batt__saves_game_when_switching_ram_banks(void)
{
  uint8_t data;

  register_callbacks();
  load_rom();

  /* Switch to Advanced addressing mode */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x6000, 0x01));

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  /* Write to all banks */
  for (uint8_t bank_num = 0; bank_num < 4; bank_num++)
  {
    for (uint16_t address = 0xA000; address < 0xC000; address++)
    {
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0xB0 | bank_num));
    }

    TEST_ASSERT_EQUAL_INT(bank_num, save_game_called_num);
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x4000, (bank_num + 1) % 4));
    TEST_ASSERT_EQUAL_INT(bank_num + 1, save_game_called_num);
  }
}

void test_mbc1_ram_batt__saves_game_on_cleanup(void)
{
  uint8_t data;

  register_callbacks();
  load_rom();

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  /* Write data to external RAM */
  for (uint16_t address = 0xA000; address < 0xC000; address++)
  {
    TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, address, 0xB0));
  }

  /* Cleanup and verify the save game callback has been called */
  TEST_ASSERT_EQUAL_INT(0, save_game_called_num);
  cleanup();
  TEST_ASSERT_EQUAL_INT(1, save_game_called_num);
}

void test_mbc1_ram_batt__does_not_save_game_when_ram_content_is_unchanged(void)
{
  uint8_t data;

  register_callbacks();
  load_rom();

  /* Enable RAM */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&mbc.bus_interface, 0x0000, 0x0A));

  /* Switch to different banks without writing */
  for (uint8_t bank_num = 0; bank_num < 4; bank_num++)
  {
    for (uint16_t address = 0xA000; address < 0xC000; address++)
    {
      TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&mbc.bus_interface, address, &data));
    }
    switch_ram_bank(&mbc, bank_num);
    TEST_ASSERT_EQUAL_INT(0, save_game_called_num);
  }

  cleanup();
  TEST_ASSERT_EQUAL_INT(0, save_game_called_num);
}
