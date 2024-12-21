#include "timer.h"

#include <stdint.h>
#include "interrupt.h"
#include "status_code.h"

status_code_t timer_init(timer_handle_t *const tmr_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(tmr_handle);

  tmr_handle->div = 0xABCC;

  return STATUS_OK;
}

status_code_t timer_tick(timer_handle_t *const tmr_handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(tmr_handle);

  status_code_t status = STATUS_OK;
  uint16_t prev_div = tmr_handle->div++;
  uint8_t timer_update = 0;

  switch ((tmr_handle->tac & 0x3)) // TODO: use constant
  {
  case 0: // TODO: use enum
    timer_update = (prev_div & tmr_handle->div & (1 << 9)) ? 1 : 0;
    break;
  case 1:
    timer_update = (prev_div & tmr_handle->div & (1 << 3)) ? 1 : 0;
    break;
  case 2:
    timer_update = (prev_div & tmr_handle->div & (1 << 5)) ? 1 : 0;
    break;
  case 3:
    timer_update = (prev_div & tmr_handle->div & (1 << 7)) ? 1 : 0;
    break;
  default:
    break;
  }

  if (timer_update && (tmr_handle->tac & 0x4)) // TODO: use constant
  {
    tmr_handle->tima++;

    if (tmr_handle->tima == 0xFF)
    {
      tmr_handle->tima = tmr_handle->tma;
      status = request_interrupt(tmr_handle->int_handle, INT_TIMER);
    }
  }

  return STATUS_OK;
}

status_code_t timer_read(timer_handle_t *const tmr_handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(tmr_handle);
  switch (address - tmr_handle->addr_offset)
  {
  case 0:
    *data = (tmr_handle->div >> 8);
    break;
  case 1:
    *data = tmr_handle->tima;
    break;
  case 2:
    *data = tmr_handle->tma;
    break;
  case 3:
    *data = tmr_handle->tac;
    break;
  default:
    break;
  }

  return STATUS_OK;
}

status_code_t timer_write(timer_handle_t *const tmr_handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(tmr_handle);

  switch (address - tmr_handle->addr_offset)
  {
  case 0:
    tmr_handle->div = 0;
    break;
  case 1:
    tmr_handle->tima = data;
    break;
  case 2:
    tmr_handle->tma = data;
    break;
  case 3:
    tmr_handle->tac = data;
    break;
  default:
    break;
  }

  return STATUS_OK;
}
