#ifndef __DMG_DATA_BUS_H__
#define __DMG_DATA_BUS_H__

#include <stdint.h>
#include "bus_interface.h"
#include "status_code.h"

typedef enum
{
  SEGMENT_TYPE_ROM_BANK_0,
  SEGMENT_TYPE_ROM_SWITCHABLE_BANK,
  SEGMENT_TYPE_VRAM,
  SEGMENT_TYPE_EXT_RAM,
  SEGMENT_TYPE_WRAM,
  SEGMENT_TYPE_OAM,
  SEGMENT_TYPE_IO_REG,
  SEGMENT_TYPE_HRAM,
  SEGMENT_TYPE_IE_REG,
  MAX_SEGMENT_TYPE,
} data_bus_segment_type_t;

typedef struct
{
  data_bus_segment_type_t segment_type;
  bus_interface_t interface;
} data_bus_segment_t;

typedef struct
{
  data_bus_segment_t segments[MAX_SEGMENT_TYPE];
} data_bus_handle_t;

status_code_t data_bus_init(data_bus_handle_t *const bus_handle);
status_code_t data_bus_add_segment(
    data_bus_handle_t *const bus_handle,
    data_bus_segment_type_t const segment_type,
    bus_interface_t const bus_interface);
status_code_t data_bus_read(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t *const data);
status_code_t data_bus_write(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_DATA_BUS_H__ */
