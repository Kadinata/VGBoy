#include "interrupt.h"

#include <stdint.h>
#include "status_code.h"

status_code_t request_interrupt(interrupt_handle_t *const int_handle, interrupt_type_t const int_type)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  int_handle->int_requested_flag |= int_type;

  return STATUS_OK;
}

status_code_t int_read_enabled_mask(interrupt_handle_t *const int_handle, uint16_t const __attribute__((unused)) address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  *data = int_handle->int_enable_mask;

  return STATUS_OK;
}

status_code_t int_write_enabled_mask(interrupt_handle_t *const int_handle, uint16_t const __attribute__((unused)) address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  int_handle->int_enable_mask = data;

  return STATUS_OK;
}
