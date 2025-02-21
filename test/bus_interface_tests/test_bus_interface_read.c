#include "unity.h"
#include "bus_interface.h"
#include "status_code.h"

#include "bus_interface_test_helper.h"

TEST_FILE("bus_interface.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void test_bus_interface_read_null_ptr(void)
{
  uint8_t data;
  bus_interface_t bus_interface = {};
  bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, NULL);

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_read(NULL, 0x1234, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_read(&bus_interface, 0x1234, NULL));
}

void test_bus_interface_read_not_initialized(void)
{
  uint8_t data;
  bus_interface_t bus_interface = {0};

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NOT_INITIALIZED, bus_interface_read(&bus_interface, 0x1234, &data));
}

void test_bus_interface_read(void)
{
  uint8_t data;
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, &ctx);

  ctx.read_data = 0x12;
  bus_interface.offset = 0x1200;

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&bus_interface, 0x1234, &data));

  /* Verify the underlying read function has been called */
  TEST_ASSERT_EQUAL_INT(1, ctx.read_count);

  /* Verify the underlying write function has not been affected */
  TEST_ASSERT_EQUAL_INT(0, ctx.write_count);

  /* Verify the read address has been forwarded to the underlying read function and the offset is applied */
  TEST_ASSERT_EQUAL_HEX16(0x0034, ctx.address);

  /* Verify the read data has been returned through pointer */
  TEST_ASSERT_EQUAL_HEX8(0x12, data);
}

void test_bus_interface_read_address_out_of_bound(void)
{
  uint8_t data;
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  ctx.return_status = STATUS_ERR_GENERIC;

  bus_interface.offset = 0x4000;

  bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, &ctx);
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_ADDRESS_OUT_OF_BOUND, bus_interface_read(&bus_interface, 0x1234, &data));
}

void test_bus_interface_read_forwards_return_status(void)
{
  uint8_t data;
  test_bus_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  ctx.return_status = STATUS_ERR_GENERIC;

  bus_interface_init(&bus_interface, stub_bus_read, stub_bus_write, &ctx);
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, bus_interface_read(&bus_interface, 0x1234, &data));
}
