#ifndef __DMG_TIMER_H__
#define __DMG_TIMER_H__

#include <stdint.h>
#include "status_code.h"
#include "interrupt.h"

typedef struct
{
  uint16_t addr_offset;
  uint16_t div;
  uint8_t tima;
  uint8_t tma;
  uint8_t tac;
  interrupt_handle_t * int_handle;
} timer_handle_t;

status_code_t timer_init(timer_handle_t *const tmr_handle);
status_code_t timer_tick(timer_handle_t *const tmr_handle);

status_code_t timer_read(timer_handle_t *const tmr_handle, uint16_t const address, uint8_t *const data);
status_code_t timer_write(timer_handle_t *const tmr_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_TIMER_H__ */
