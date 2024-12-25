#include "bus_interface.h"

#include <stdint.h>
#include "status_code.h"

status_code_t bus_interface_read(bus_interface_t *const bus_interface, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(bus_interface->read, STATUS_ERR_NOT_INITIALIZED);

  return bus_interface->read(bus_interface->resource, address, data);
}

status_code_t bus_interface_write(bus_interface_t *const bus_interface, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(bus_interface->write, STATUS_ERR_NOT_INITIALIZED);

  return bus_interface->write(bus_interface->resource, address, data);
}
