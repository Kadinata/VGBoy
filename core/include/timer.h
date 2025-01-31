#ifndef __DMG_TIMER_H__
#define __DMG_TIMER_H__

#include <stdint.h>
#include "status_code.h"
#include "bus_interface.h"
#include "interrupt.h"

typedef enum
{
  TMR_TAC_CLK_SEL = 0x3,
  TMR_TAC_ENABLE = (1 << 2),
} timer_reg_tac_t;

typedef struct
{
  uint16_t div;
  uint8_t tima;
  uint8_t tma;
  uint8_t tac;
} timer_registers_t;

typedef struct
{
  timer_registers_t registers;
  interrupt_handle_t *interrupt;
  bus_interface_t bus_interface;
} timer_handle_t;

status_code_t timer_init(timer_handle_t *const timer, interrupt_handle_t *const interrupt);
status_code_t timer_tick(timer_handle_t *const timer);

#endif /* __DMG_TIMER_H__ */
