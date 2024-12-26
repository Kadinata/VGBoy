#include "oam.h"

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

static status_code_t oam_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t oam_write(void *const resource, uint16_t const address, uint8_t const data);

/**
 * TODO: Disable OAM read/write by the CPU during DMA transfer
 * read returns 0xFF
 * write is a nop
 */

status_code_t oam_init(oam_handle_t *const oam_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);

  oam_handle->bus_interface.read = oam_read;
  oam_handle->bus_interface.write = oam_write;
  oam_handle->bus_interface.resource = oam_handle;

  return STATUS_OK;
}

static status_code_t oam_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  oam_handle_t *const oam_handle = (oam_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = oam_handle->oam_buf[address];

  return STATUS_OK;
}

static status_code_t oam_write(void *const resource, uint16_t const address, uint8_t const data)
{
  oam_handle_t *const oam_handle = (oam_handle_t *)resource;
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  oam_handle->oam_buf[address] = data;

  return STATUS_OK;
}
