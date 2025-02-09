#include "unity.h"

#include <string.h>

#include "mbc.h"
#include "mock_bus_interface.h"
#include "mock_rom.h"
#include "mock_rtc.h"

TEST_FILE("mbc.c")

static mbc_handle_t mbc;

void setup(void)
{
  memset(&mbc, 0, sizeof(mbc_handle_t));
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
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, mbc_init(NULL));
}

void test_mbc_init__forwards_errors(void)
{
  bus_interface_init_ExpectAndReturn(&mbc.bus_interface, NULL, NULL, &mbc, STATUS_ERR_GENERIC);
  bus_interface_init_IgnoreArg_read_fn();
  bus_interface_init_IgnoreArg_write_fn();
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, mbc_init(&mbc));
}
