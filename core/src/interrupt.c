#include "interrupt.h"

#include <stdint.h>
#include "status_code.h"

static interrupt_vector_t interrupt_vector_table[] = {
    {.address = 0x0040, .int_type = INT_VBLANK},
    {.address = 0x0048, .int_type = INT_LCD},
    {.address = 0x0050, .int_type = INT_TIMER},
    {.address = 0x0058, .int_type = INT_SERIAL},
    {.address = 0x0060, .int_type = INT_JOYPAD},
};

static uint8_t handle_single_interrupt(interrupt_handle_t *const int_handle, interrupt_vector_t *const int_vector);

status_code_t interrupt_init(interrupt_handle_t *const int_handle, interrupt_handler_cb_fn const callback_fn, void *callback_ctx)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback_fn);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(int_handle->callback.callback_fn != NULL, STATUS_ERR_ALREADY_INITIALIZED);

  int_handle->regs.ime = 0;
  int_handle->regs.irf = 0;
  int_handle->regs.ien = 0;

  int_handle->callback.callback_fn = callback_fn;
  int_handle->callback.callback_ctx = callback_ctx;

  return STATUS_OK;
}

status_code_t request_interrupt(interrupt_handle_t *const int_handle, interrupt_type_t const int_type)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  int_handle->regs.irf |= int_type;

  return STATUS_OK;
}

status_code_t service_interrupt(interrupt_handle_t *const int_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  for (int8_t i = 0; i < 5; i++)
  {
    if (handle_single_interrupt(int_handle, &interrupt_vector_table[i]))
    {
      interrupt_callback_t *const callback = &int_handle->callback;
      VERIFY_COND_RETURN_STATUS_IF_TRUE(callback->callback_fn == NULL, STATUS_OK);
      return callback->callback_fn(callback->callback_ctx, interrupt_vector_table[i].address);
    }
  }
  return STATUS_OK;
}

status_code_t int_read_enabled_mask(interrupt_handle_t *const int_handle, uint16_t const __attribute__((unused)) address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  *data = int_handle->regs.ien;

  return STATUS_OK;
}

status_code_t int_write_enabled_mask(interrupt_handle_t *const int_handle, uint16_t const __attribute__((unused)) address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  int_handle->regs.ien = data;

  return STATUS_OK;
}

static uint8_t handle_single_interrupt(interrupt_handle_t *const int_handle, interrupt_vector_t *const int_vector)
{
  if (!(int_handle->regs.ien & int_handle->regs.irf & int_vector->int_type))
  {
    return 0;
  }

  /* Clear the request flag for this inetrrupt */
  int_handle->regs.irf &= ~int_vector->int_type;

  /* Disable interrupt master enable */
  int_handle->regs.ime = 0;

  return 1;
}
