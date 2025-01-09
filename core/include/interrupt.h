#ifndef __DMG_INTERRRUPT_H__
#define __DMG_INTERRRUPT_H__

#include <stdint.h>
#include "bus_interface.h"
#include "callback.h"
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
  uint8_t ime;
  uint8_t irf;
  uint8_t ien;
} interrupt_registers_t;

/* Struct containing all data necessary to manage interrupts */
typedef struct
{
  interrupt_registers_t regs;
  callback_t *callback;
  bus_interface_t bus_interface;
} interrupt_handle_t;

status_code_t interrupt_init(interrupt_handle_t *const int_handle, callback_t *const interrupt_cb);

status_code_t request_interrupt(interrupt_handle_t *const int_handle, interrupt_type_t const int_type);
status_code_t service_interrupt(interrupt_handle_t *const int_handle);

#endif /* __DMG_INTERRRUPT_H__ */
