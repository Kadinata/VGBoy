#ifndef __DMG_DATA_BUS_H__
#define __DMG_DATA_BUS_H__

#include <stdint.h>
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

typedef status_code_t (*data_bus_segment_read_fn)(void *const resource, uint16_t const address, uint8_t *const data);
typedef status_code_t (*data_bus_segment_write_fn)(void *const resource, uint16_t const address, uint8_t const data);

typedef struct
{
  data_bus_segment_type_t segment_type;
  data_bus_segment_read_fn read_fn;
  data_bus_segment_write_fn write_fn;
  void *resource;
} data_bus_segment_t;

typedef struct
{
  data_bus_segment_t segments[MAX_SEGMENT_TYPE];
} data_bus_handle_t;

status_code_t data_bus_init(data_bus_handle_t *const bus_handle);
status_code_t data_bus_add_segment(
    data_bus_handle_t *const bus_handle,
    data_bus_segment_type_t const segment_type,
    data_bus_segment_read_fn const read_fn,
    data_bus_segment_write_fn const write_fn,
    void *const bus_resource);
status_code_t data_bus_read(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t *const data);
status_code_t data_bus_write(data_bus_handle_t *const bus_handle, uint16_t const address, uint8_t const data);

#endif /* __DMG_DATA_BUS_H__ */
