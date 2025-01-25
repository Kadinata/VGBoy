#include "bus_interface_test_helper.h"

status_code_t stub_bus_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  test_bus_data_ctx_t *ctx = (test_bus_data_ctx_t *)resource;
  ctx->address = address;
  *data = ctx->read_data;
  ctx->read_count++;

  return ctx->return_status;
}

status_code_t stub_bus_write(void *const resource, uint16_t const address, uint8_t const data)
{
  test_bus_data_ctx_t *ctx = (test_bus_data_ctx_t *)resource;
  ctx->address = address;
  ctx->write_data = data;
  ctx->write_count++;

  return ctx->return_status;
}
