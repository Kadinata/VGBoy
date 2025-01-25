#ifndef __BUS_INTERFACE_TEST_HELPER_H__
#define __BUS_INTERFACE_TEST_HELPER_H__

#include "bus_interface.h"

typedef struct
{
  uint8_t read_data;
  uint8_t write_data;
  uint8_t read_count;
  uint8_t write_count;
  uint16_t address;
  status_code_t return_status;
} test_bus_data_ctx_t;

status_code_t stub_bus_read(void *const resource, uint16_t const address, uint8_t *const data);
status_code_t stub_bus_write(void *const resource, uint16_t const address, uint8_t const data);

#endif /* __BUS_INTERFACE_TEST_HELPER_H__ */
