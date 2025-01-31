#include "timer.h"

#include <stdint.h>
#include "interrupt.h"
#include "bus_interface.h"
#include "status_code.h"

static status_code_t timer_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t timer_write(void *const resource, uint16_t const address, uint8_t const data);

status_code_t timer_init(timer_handle_t *const timer, interrupt_handle_t *const interrupt)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(timer);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(interrupt);

  timer->registers.div = 0xABCC;
  timer->interrupt = interrupt;

  return bus_interface_init(&timer->bus_interface, timer_read, timer_write, timer);
}

status_code_t timer_tick(timer_handle_t *const timer)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(timer);

  static const uint16_t clock_select_mask[] = {(1 << 9), (1 << 3), (1 << 5), (1 << 7)};
  status_code_t status = STATUS_OK;

  /**
   * Always increment timer DIV at every T-cycle tick
   * and determine which bits flipped from 1 to 0.
   */
  uint16_t bit_changes = (timer->registers.div++) & (~timer->registers.div);

  uint8_t clock_select = timer->registers.tac & TMR_TAC_CLK_SEL;
  uint8_t timer_update = !!(bit_changes & clock_select_mask[clock_select]);

  if (!(timer->registers.tac & TMR_TAC_ENABLE) || !timer_update)
  {
    return STATUS_OK;
  }

  timer->registers.tima++;

  /* Reload TIMA and request interrupt when it overflows */
  if (timer->registers.tima == 0xFF)
  {
    timer->registers.tima = timer->registers.tma;
    status = request_interrupt(timer->interrupt, INT_TIMER);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return STATUS_OK;
}

static status_code_t timer_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  timer_handle_t *timer = (timer_handle_t *)resource;

  switch (address)
  {
  case 0:
    *data = (timer->registers.div >> 8);
    break;
  case 1:
    *data = timer->registers.tima;
    break;
  case 2:
    *data = timer->registers.tma;
    break;
  case 3:
    *data = timer->registers.tac;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
    break;
  }

  return STATUS_OK;
}

static status_code_t timer_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  timer_handle_t *timer = (timer_handle_t *)resource;

  switch (address)
  {
  case 0:
    timer->registers.div = 0;
    break;
  case 1:
    timer->registers.tima = data;
    break;
  case 2:
    timer->registers.tma = data;
    break;
  case 3:
    timer->registers.tac = data;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
    break;
  }

  return STATUS_OK;
}
