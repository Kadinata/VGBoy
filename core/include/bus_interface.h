#ifndef __DMG_BUS_INTERFACE_H__
#define __DMG_BUS_INTERFACE_H__

#include <stdint.h>
#include "status_code.h"

typedef status_code_t (*bus_read_fn)(void *const resource, uint16_t const address, uint8_t *const data);
typedef status_code_t (*bus_write_fn)(void *const resource, uint16_t const address, uint8_t const data);

typedef struct
{
  uint16_t offset;
  bus_read_fn read;
  bus_write_fn write;
  void *resource;
} bus_interface_t;

status_code_t bus_interface_read(bus_interface_t *const bus_interface, uint16_t const address, uint8_t *const data);
status_code_t bus_interface_write(bus_interface_t *const bus_interface, uint16_t const address, uint8_t const data);

#endif /* __DMG_BUS_INTERFACE_H__ */
