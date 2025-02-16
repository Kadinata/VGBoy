#include "unity.h"

#include <string.h>

#include "mbc.h"
#include "mbc_test_helper.h"

#include "mock_bus_interface.h"
#include "mock_rom.h"
#include "mock_rtc.h"

TEST_FILE("mbc.c")

static mbc_handle_t mbc;
static uint8_t *rom_data;

void setup(void)
{
  memset(&mbc, 0, sizeof(mbc_handle_t));

  rom_data = create_rom(ROM_ONLY, 0x00, MBC_EXT_RAM_SIZE_NO_RAM);
  TEST_ASSERT_NOT_NULL(rom_data);
}

void tearDown(void)
{
}

void test_mbc_init(void)
{
  bus_interface_init_ExpectAndReturn(&mbc.bus_interface, NULL, NULL, &mbc, STATUS_OK);
  bus_interface_init_IgnoreArg_read_fn();
  bus_interface_init_IgnoreArg_write_fn();
  TEST_ASSERT_EQUAL_INT(STATUS_OK, mbc_init(&mbc));
}

void test_mbc_init__null_ptr(void)
{
  mbc_callbacks_t mbc_callbacks = {0};

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_init(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_load_rom(NULL, rom_data, 0x8000));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_register_callbacks(NULL, &mbc_callbacks));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_register_callbacks(&mbc, NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_cleanup(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_save_game(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_load_saved_game(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_reload_banks(NULL));
}

void test_mbc_init__forwards_errors(void)
{
  bus_interface_init_ExpectAndReturn(&mbc.bus_interface, NULL, NULL, &mbc, STATUS_ERR_GENERIC);
  bus_interface_init_IgnoreArg_read_fn();
  bus_interface_init_IgnoreArg_write_fn();
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, mbc_init(&mbc));
}
