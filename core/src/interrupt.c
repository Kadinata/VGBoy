#include "interrupt.h"

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

#define REG_IEN_OFFSET (0xFF)
#define REG_IRF_OFFSET (0x0F)

static interrupt_vector_t interrupt_vector_table[] = {
    {.address = 0x0040, .int_type = INT_VBLANK},
    {.address = 0x0048, .int_type = INT_LCD},
    {.address = 0x0050, .int_type = INT_TIMER},
    {.address = 0x0058, .int_type = INT_SERIAL},
    {.address = 0x0060, .int_type = INT_JOYPAD},
};

static status_code_t interrupt_reg_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t interrupt_reg_write(void *const resource, uint16_t const address, uint8_t const data);
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

  return bus_interface_init(&int_handle->bus_interface, interrupt_reg_read, interrupt_reg_write, int_handle);
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

static status_code_t interrupt_reg_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  interrupt_handle_t *const int_handle = (interrupt_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  switch (address)
  {
  case REG_IRF_OFFSET:
    *data = int_handle->regs.irf;
    break;
  case REG_IEN_OFFSET:
    *data = int_handle->regs.ien;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}

static status_code_t interrupt_reg_write(void *const resource, uint16_t const address, uint8_t const data)
{
  interrupt_handle_t *const int_handle = (interrupt_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(int_handle);

  switch (address)
  {
  case REG_IRF_OFFSET:
    int_handle->regs.irf = data;
    break;
  case REG_IEN_OFFSET:
    int_handle->regs.ien = data;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

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
