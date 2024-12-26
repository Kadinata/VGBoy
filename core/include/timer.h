#ifndef __DMG_TIMER_H__
#define __DMG_TIMER_H__

#include <stdint.h>
#include "status_code.h"
#include "bus_interface.h"
#include "interrupt.h"

typedef struct
{
  uint16_t div;
  uint8_t tima;
  uint8_t tma;
  uint8_t tac;
  interrupt_handle_t *int_handle;
  bus_interface_t bus_interface;
} timer_handle_t;

status_code_t timer_init(timer_handle_t *const tmr_handle, interrupt_handle_t *const int_handle);
status_code_t timer_tick(timer_handle_t *const tmr_handle);

#endif /* __DMG_TIMER_H__ */
