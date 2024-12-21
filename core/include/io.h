#ifndef __DMG_IO_H__
#define __DMG_IO_H__

#include <stdint.h>
#include "status_code.h"
#include "timer.h"
#include "interrupt.h"

typedef struct
{
  uint16_t offset;
  timer_handle_t *timer_handle;
  interrupt_handle_t *int_handle;
} io_handle_t;

status_code_t io_read(io_handle_t *const io_handle, uint16_t const address, uint8_t *const data);
status_code_t io_write(io_handle_t *const io_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_IO_H__ */
