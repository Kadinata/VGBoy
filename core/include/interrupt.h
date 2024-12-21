#ifndef __DMG_INTERRRUPT_H__
#define __DMG_INTERRRUPT_H__

#include <stdint.h>
#include "status_code.h"

typedef enum
{
  INT_VBLANK = (1 << 0),
  INT_LCD = (1 << 1),
  INT_TIMER = (1 << 2),
  INT_SERIAL = (1 << 3),
  INT_JOYPAD = (1 << 4),
} interrupt_type_t;

typedef struct
{
  interrupt_type_t int_type;
  uint16_t address;
} interrupt_vector_t;

typedef struct
{
  uint8_t int_requested_flag;
  uint8_t int_enable_mask;
} interrupt_handle_t;

status_code_t request_interrupt(interrupt_handle_t *const int_handle, interrupt_type_t const int_type);

#endif /* __DMG_INTERRRUPT_H__ */
