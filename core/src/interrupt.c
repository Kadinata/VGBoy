#include "interrupt.h"

#include <stdint.h>
#include "status_code.h"

status_code_t request_interrupt(interrupt_handle_t *const int_handle, interrupt_type_t const int_type)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  int_handle->int_requested_flag |= int_type;

  return STATUS_OK;
}
