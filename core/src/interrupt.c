#include "interrupt.h"

#include <stdint.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "callback.h"
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
static inline bool handle_single_interrupt(interrupt_handle_t *const interrupt, interrupt_vector_t *const int_vector);

status_code_t interrupt_init(interrupt_handle_t *const interrupt, callback_t *const interrupt_cb)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt_cb);

  interrupt->registers.ime = 0;
  interrupt->registers.irf = 0;
  interrupt->registers.ien = 0;
  interrupt->callback = interrupt_cb;

  return bus_interface_init(&interrupt->bus_interface, interrupt_reg_read, interrupt_reg_write, interrupt);
}

bool has_pending_interrupts(interrupt_handle_t *const interrupt)
{
  return (interrupt && interrupt->registers.irf);
}

bool interrupt_globally_enabled(interrupt_handle_t *const interrupt)
{
  return (interrupt && interrupt->registers.ime);
}

status_code_t global_interrupt_enable(interrupt_handle_t *const interrupt, bool enable)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);

  interrupt->registers.ime = enable;

  return STATUS_OK;
}

status_code_t request_interrupt(interrupt_handle_t *const interrupt, interrupt_type_t const int_type)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);

  interrupt->registers.irf |= int_type;

  return STATUS_OK;
}

status_code_t service_interrupt(interrupt_handle_t *const interrupt)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);

  for (int8_t i = 0; i < 5; i++)
  {
    if (handle_single_interrupt(interrupt, &interrupt_vector_table[i]))
    {
      uint16_t const isr_address = interrupt_vector_table[i].address;
      VERIFY_COND_RETURN_STATUS_IF_TRUE(interrupt->callback->callback_fn == NULL, STATUS_OK);
      return callback_call(interrupt->callback, &isr_address);
    }
  }
  return STATUS_OK;
}

static status_code_t interrupt_reg_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  interrupt_handle_t *const interrupt = (interrupt_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  switch (address)
  {
  case REG_IRF_OFFSET:
    *data = interrupt->registers.irf;
    break;
  case REG_IEN_OFFSET:
    *data = interrupt->registers.ien;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}

static status_code_t interrupt_reg_write(void *const resource, uint16_t const address, uint8_t const data)
{
  interrupt_handle_t *const interrupt = (interrupt_handle_t *)resource;

  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);

  switch (address)
  {
  case REG_IRF_OFFSET:
    interrupt->registers.irf = data;
    break;
  case REG_IEN_OFFSET:
    interrupt->registers.ien = data;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}

static inline bool handle_single_interrupt(interrupt_handle_t *const interrupt, interrupt_vector_t *const int_vector)
{
  if (!(interrupt->registers.ien & interrupt->registers.irf & int_vector->int_type))
  {
    return false;
  }

  /* Clear the request flag for this inetrrupt */
  interrupt->registers.irf &= ~int_vector->int_type;

  /* Disable interrupt master enable */
  interrupt->registers.ime = 0;

  return true;
}
