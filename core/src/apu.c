#include "apu.h"

#include <stdint.h>

#include "bus_interface.h"
#include "logging.h"
#include "status_code.h"

static status_code_t apu_bus_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_bus_write(void *const resource, uint16_t const address, uint8_t const data);

status_code_t apu_init(apu_handle_t *const apu)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu);
  return bus_interface_init(&apu->bus_interface, apu_bus_read, apu_bus_write, apu);
}

static status_code_t apu_bus_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  apu_handle_t *const apu = (apu_handle_t *)resource;
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(apu->registers), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = apu->registers.buffer[address];
  return STATUS_OK;
}

static status_code_t apu_bus_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  apu_handle_t *const apu = (apu_handle_t *)resource;
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(apu->registers), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  apu->registers.buffer[address] = data;
  return STATUS_OK;
}
