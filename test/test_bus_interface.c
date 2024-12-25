#include "unity.h"
#include "bus_interface.h"
#include "status_code.h"

TEST_FILE("bus_interface.c")

typedef struct
{
  uint8_t read_data;
  uint8_t write_data;
  uint8_t read_count;
  uint8_t write_count;
  uint16_t address;
  status_code_t return_status;
} test_data_ctx_t;

void setUp(void)
{
}

void tearDown(void)
{
}

status_code_t stub_bus_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  test_data_ctx_t *ctx = (test_data_ctx_t *)resource;
  ctx->address = address;
  *data = ctx->read_data;
  ctx->read_count++;

  return ctx->return_status;
}

status_code_t stub_bus_write(void *const resource, uint16_t const address, uint8_t const data)
{
  test_data_ctx_t *ctx = (test_data_ctx_t *)resource;
  ctx->address = address;
  ctx->write_data = data;
  ctx->write_count++;

  return ctx->return_status;
}

void stub_init_bus_interface(bus_interface_t *bus_interface, test_data_ctx_t *ctx)
{
  bus_interface->read = stub_bus_read;
  bus_interface->write = stub_bus_write;
  bus_interface->resource = ctx;

  if (ctx)
  {
    ctx->return_status = STATUS_OK;
  }
}

void test_bus_interface_read_null_ptr(void)
{
  uint8_t data;
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, NULL);

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_read(NULL, 0x1234, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_read(&bus_interface, 0x1234, NULL));
}

void test_bus_interface_write_null_ptr(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, bus_interface_write(NULL, 0x1234, 0x12));
}

void test_bus_interface_read_not_initialized(void)
{
  uint8_t data;
  bus_interface_t bus_interface = {0};

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NOT_INITIALIZED, bus_interface_read(&bus_interface, 0x1234, &data));
}

void test_bus_interface_write_not_initialized(void)
{
  bus_interface_t bus_interface = {0};
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NOT_INITIALIZED, bus_interface_write(&bus_interface, 0x1234, 0x12));
}

void test_bus_interface_read(void)
{
  uint8_t data;
  test_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, &ctx);

  ctx.read_data = 0x12;

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_read(&bus_interface, 0x1234, &data));

  /* Verify the underlying read function has been called */
  TEST_ASSERT_EQUAL_INT(1, ctx.read_count);

  /* Verify the underlying write function has not been affected */
  TEST_ASSERT_EQUAL_INT(0, ctx.write_count);

  /* Verify the read address has been forwarded to the underlying read function */
  TEST_ASSERT_EQUAL_HEX16(0x1234, ctx.address);

  /* Verify the read data has been returned through pointer */
  TEST_ASSERT_EQUAL_HEX8(0x12, data);
}

void test_bus_interface_write(void)
{
  test_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, &ctx);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, bus_interface_write(&bus_interface, 0x1234, 0x12));

  /* Verify the underlying write function has been called */
  TEST_ASSERT_EQUAL_INT(1, ctx.write_count);

  /* Verify the underlying read function has not been affected */
  TEST_ASSERT_EQUAL_INT(0, ctx.read_count);

  /* Verify the read address has been forwarded to the underlying write function */
  TEST_ASSERT_EQUAL_HEX16(0x1234, ctx.address);

  /* Verify the write data has been written */
  TEST_ASSERT_EQUAL_HEX8(0x12, ctx.write_data);
}

void test_bus_interface_forwards_return_status(void)
{
  uint8_t data;
  test_data_ctx_t ctx = {0};
  bus_interface_t bus_interface = {0};
  stub_init_bus_interface(&bus_interface, &ctx);

  ctx.return_status = STATUS_ERR_GENERIC;

  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, bus_interface_read(&bus_interface, 0x1234, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_GENERIC, bus_interface_write(&bus_interface, 0x1234, 0x12));
}
