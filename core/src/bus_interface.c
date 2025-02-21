#include "bus_interface.h"

#include <stdint.h>
#include "status_code.h"

status_code_t bus_interface_init(bus_interface_t *const bus_interface, const bus_read_fn read_fn, const bus_write_fn write_fn, void *const resource)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(read_fn);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(write_fn);

  bus_interface->read = read_fn;
  bus_interface->write = write_fn;
  bus_interface->resource = resource;

  return STATUS_OK;
}

status_code_t bus_interface_read(bus_interface_t *const bus_interface, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(bus_interface->read, STATUS_ERR_NOT_INITIALIZED);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address < bus_interface->offset, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  return bus_interface->read(bus_interface->resource, address - bus_interface->offset, data);
}

status_code_t bus_interface_write(bus_interface_t *const bus_interface, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(bus_interface->write, STATUS_ERR_NOT_INITIALIZED);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address < bus_interface->offset, STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  return bus_interface->write(bus_interface->resource, address - bus_interface->offset, data);
}
