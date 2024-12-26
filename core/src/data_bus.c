#include "data_bus.h"

#include <stdint.h>
#include <string.h>

#include "logging.h"
#include "status_code.h"

static status_code_t unsupported_region_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t unsupported_region_write(void *const resource, uint16_t const address, uint8_t const data);

static status_code_t data_bus_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t data_bus_write(void *const resource, uint16_t const address, uint8_t const data);

static data_bus_segment_t unsupported_region = (data_bus_segment_t){
    .segment_type = -1,
    .interface = (bus_interface_t){
        .read = unsupported_region_read,
        .write = unsupported_region_write,
        .resource = NULL,
    },
};

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
  /* 0x8000 - 0x9FFF: Video RAM (VRAM) */
  else if ((address >= 0x8000) && (address < 0xA000))
  {
    return &handle->segments[SEGMENT_TYPE_VRAM];
  }
  /* 0xA000 - 0xBFFF: External RAM */
  else if ((address >= 0xA000) && (address < 0xC000))
  {
    return &handle->segments[SEGMENT_TYPE_EXT_RAM];
  }
  /* 0xC000 - 0xDFFF: Working RAM (WRAM) */
  else if ((address >= 0xC000) && (address < 0xE000))
  {
    return &handle->segments[SEGMENT_TYPE_WRAM];
  }
  /* 0xE000 - 0xFDFF: Echo RAM (region not supported) */
  else if ((address >= 0xE000) && (address < 0xFE00))
  {
    return &unsupported_region;
  }
  /* 0xFE00 - 0xFE9F: Object Attribute Memory (OAM) */
  else if ((address >= 0xFE00) && (address < 0xFEA0))
  {
    return &handle->segments[SEGMENT_TYPE_OAM];
  }
  /* 0xFEA0 - 0xFEFF: Not usable */
  else if ((address >= 0xFEA0) && (address < 0xFF00))
  {
    return &unsupported_region;
  }
  /* 0xFF00 - 0xFF7F: I/O Registers */
  else if ((address >= 0xFF00) && (address < 0xFF80))
  {
    return &handle->segments[SEGMENT_TYPE_IO_REG];
  }
  /* 0xFF80 - 0xFFFE: High RAM (HRAM) */
  else if ((address >= 0xFF80) && (address < 0xFFFF))
  {
    return &handle->segments[SEGMENT_TYPE_HRAM];
  }
  /* 0xFFFF: Interrupt Enable Register (IE) */
  else if (address == 0xFFFF)
  {
    return &handle->segments[SEGMENT_TYPE_IE_REG];
  }

  return NULL;
}

// TODO: may remove
status_code_t data_bus_init(data_bus_handle_t *const bus_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);

  bus_handle->bus_interface.read = data_bus_read;
  bus_handle->bus_interface.write = data_bus_write;
  bus_handle->bus_interface.resource = bus_handle;

  return STATUS_OK;
}

status_code_t data_bus_add_segment(
    data_bus_handle_t *const bus_handle,
    data_bus_segment_type_t const segment_type,
    bus_interface_t const bus_interface)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface.read);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface.write);

  bus_handle->segments[segment_type].segment_type = segment_type;
  memcpy(&bus_handle->segments[segment_type].interface, &bus_interface, sizeof(bus_interface_t));

  return STATUS_OK;
}

static status_code_t data_bus_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  data_bus_handle_t *const bus_handle = (data_bus_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  data_bus_segment_t *bus_segment = get_bus_segment(bus_handle, address);

  if (bus_segment == NULL || bus_segment->interface.read == NULL)
  {
    Log_E("Bus segment not initialized for address: 0x%04X", address);
    return STATUS_ERR_NOT_INITIALIZED;
  }

  return bus_interface_read(&bus_segment->interface, address, data);
}

static status_code_t data_bus_write(void *const resource, uint16_t const address, uint8_t const data)
{
  data_bus_handle_t *const bus_handle = (data_bus_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_handle);

  data_bus_segment_t *bus_segment = get_bus_segment(bus_handle, address);

  if (bus_segment == NULL || bus_segment->interface.write == NULL)
  {
    Log_E("Bus segment not initialized for address: 0x%04X", address);
    return STATUS_ERR_NOT_INITIALIZED;
  }

  return bus_interface_write(&bus_segment->interface, address, data);
}

static status_code_t unsupported_region_read(void *const __attribute__((unused)) resource, uint16_t const address, uint8_t *const data)
{
  Log_E("Attempted read from an unsupported address: 0x%04X", address);
  *data = 0;
  return STATUS_OK;
}

static status_code_t unsupported_region_write(void *const __attribute__((unused)) resource, uint16_t const address, uint8_t const __attribute__((unused)) data)
{
  Log_E("Attempted write to an unsupported address: 0x%04X", address);
  return STATUS_OK;
}
