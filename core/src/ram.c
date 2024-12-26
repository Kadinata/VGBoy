#include "ram.h"

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

static status_code_t ram_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t ram_write(void *const resource, uint16_t const address, uint8_t const data);

static inline uint8_t address_in_range(uint16_t address, uint16_t offset, uint16_t size)
{
  return ((address >= offset) && (address < (offset + size)));
}

status_code_t ram_init(ram_handle_t *const ram_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ram_handle);

  ram_handle->bus_interface.read = ram_read;
  ram_handle->bus_interface.write = ram_write;
  ram_handle->bus_interface.offset = 0x0000;
  ram_handle->bus_interface.resource = ram_handle;

  return STATUS_OK;
}

static status_code_t ram_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  uint16_t effective_address;
  ram_handle_t *const ram_handle = (ram_handle_t *)resource;

  // TODO: check for address bound
  if (address_in_range(address, ram_handle->wram.offset, WRAM_SIZE))
  {
    effective_address = address - ram_handle->wram.offset;
    *data = ram_handle->wram.buf[effective_address];
  }
  else if (address_in_range(address, ram_handle->vram.offset, VRAM_SIZE))
  {
    effective_address = address - ram_handle->vram.offset;
    *data = ram_handle->vram.buf[effective_address];
  }
  else if (address_in_range(address, ram_handle->hram.offset, HRAM_SIZE))
  {
    effective_address = address - ram_handle->hram.offset;
    *data = ram_handle->hram.buf[effective_address];
  }
  else
  {
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}

static status_code_t ram_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  ram_handle_t *const ram_handle = (ram_handle_t *)resource;

  uint16_t effective_address;

  // TODO: check for address bound

  if (address_in_range(address, ram_handle->wram.offset, WRAM_SIZE))
  {
    effective_address = address - ram_handle->wram.offset;
    ram_handle->wram.buf[effective_address] = data;
  }
  else if (address_in_range(address, ram_handle->vram.offset, VRAM_SIZE))
  {
    effective_address = address - ram_handle->vram.offset;
    ram_handle->vram.buf[effective_address] = data;
  }
  else if (address_in_range(address, ram_handle->hram.offset, HRAM_SIZE))
  {
    effective_address = address - ram_handle->hram.offset;
    ram_handle->hram.buf[effective_address] = data;
  }
  else
  {
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}
