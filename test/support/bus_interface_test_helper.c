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

void stub_init_bus_interface(bus_interface_t *bus_interface, test_bus_data_ctx_t *ctx)
{
  bus_interface->read = stub_bus_read;
  bus_interface->write = stub_bus_write;
  bus_interface->resource = ctx;

  if (ctx)
  {
    ctx->return_status = STATUS_OK;
  }
}
