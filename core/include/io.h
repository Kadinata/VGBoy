#ifndef __DMG_IO_H__
#define __DMG_IO_H__

#include <stdint.h>
#include "status_code.h"
#include "bus_interface.h"
#include "dma.h"

typedef struct
{
  dma_handle_t *dma_handle;
  bus_interface_t *int_bus_interface;
  bus_interface_t *lcd_bus_interface;
  bus_interface_t *timer_bus_interface;
  bus_interface_t bus_interface;
} io_handle_t;

typedef struct
{
  dma_handle_t *dma_handle;
  bus_interface_t *int_bus_interface;
  bus_interface_t *lcd_bus_interface;
  bus_interface_t *timer_bus_interface;
} io_init_param_t;

status_code_t io_init(io_handle_t *const io_handle, io_init_param_t *const init_param);

#endif /* __DMG_IO_H__ */
