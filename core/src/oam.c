#include "oam.h"

#include <stdint.h>
#include "status_code.h"

status_code_t oam_read(oam_handle_t *const oam_handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  uint16_t const effective_address = address - oam_handle->offset;

  VERIFY_COND_RETURN_STATUS_IF_TRUE(effective_address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = oam_handle->oam_buf[effective_address];

  return STATUS_OK;
}

status_code_t oam_write(oam_handle_t *const oam_handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(oam_handle);

  uint16_t const effective_address = address - oam_handle->offset;

  VERIFY_COND_RETURN_STATUS_IF_TRUE(effective_address >= sizeof(oam_handle->entries), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  oam_handle->oam_buf[effective_address] = data;

  return STATUS_OK;
}
