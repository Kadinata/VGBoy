#include "data_bus.h"

#include <stdint.h>
#include "status_code.h"

static data_bus_segment_t *get_bus_segment(data_bus_handle_t *const handle, uint16_t const address)
{
  /* 0x0000 - 0x3FFF: ROM BANK 0 */
  if (address < 0x4000)
  {
    return &handle->segments[SEGMENT_TYPE_ROM_BANK_0];
  }
  /* 0x0000 - 0x7FFF: ROM BANK 1 - N */
  else if ((address >= 0x4000) && (address < 0x8000))
  {
    return &handle->segments[SEGMENT_TYPE_ROM_SWITCHABLE_BANK];
  }

  return NULL;
}

status_code_t data_bus_init(data_bus_handle_t *const bus_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);

  return STATUS_OK;
}

status_code_t data_bus_add_segment(
    data_bus_handle_t *const bus_handle,
    data_bus_segment_type_t const segment_type,
    data_bus_segment_read_fn const read_fn,
    data_bus_segment_write_fn const write_fn,
    void *const bus_resource)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(read_fn);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(read_fn);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_resource);

  bus_handle->segments[segment_type].segment_type = segment_type;
  bus_handle->segments[segment_type].write_fn = write_fn;
  bus_handle->segments[segment_type].read_fn = read_fn;
  bus_handle->segments[segment_type].resource = bus_resource;

  return STATUS_OK;
}

status_code_t data_bus_read(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  data_bus_segment_t *bus_segment = get_bus_segment(bus_handle, address);

  if (bus_segment == NULL || bus_segment->read_fn == NULL)
  {
    return STATUS_ERR_GENERIC; /** TODO: return specific error */
  }

  bus_segment->read_fn(bus_segment->resource, address, data);
  return STATUS_OK;
}

status_code_t data_bus_write(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);

  data_bus_segment_t *bus_segment = get_bus_segment(bus_handle, address);

  if (bus_segment == NULL || bus_segment->write_fn == NULL)
  {
    return STATUS_ERR_GENERIC; /** TODO: return specific error */
  }

  bus_segment->write_fn(bus_segment->resource, address, data);
  return STATUS_OK;
}
