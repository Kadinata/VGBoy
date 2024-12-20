#include "cpu_test_helper.h"

#include <stdint.h>

#include "unity.h"
#include "status_code.h"
#include "cpu.h"
#include "mock_memory.h"

static status_code_t mock_bus_read_8(void *const resource, uint16_t addr, uint8_t *data)
{
  return mem_read_8(addr, data);
}

static status_code_t mock_bus_write_8(void *const resource, uint16_t addr, uint8_t data)
{
  return mem_write_8(addr, data);
}

void stub_cpu_state_init(cpu_state_t *state)
{
  state->registers.pc = TEST_PC_INIT_VALUE;
  state->m_cycles = 0;
  state->run_mode = RUN_MODE_NORMAL;
  state->bus_interface.read = mock_bus_read_8;
  state->bus_interface.write = mock_bus_write_8;
};

void stub_mem_read_8(uint16_t addr, uint8_t *data)
{
  mem_read_8_ExpectAndReturn(addr, NULL, STATUS_OK);
  mem_read_8_IgnoreArg_data();
  mem_read_8_ReturnThruPtr_data(data);
}

void stub_mem_read_16(uint16_t addr, uint16_t *data)
{
  mem_read_8_ExpectAndReturn(addr, NULL, STATUS_OK);
  mem_read_8_IgnoreArg_data();
  mem_read_8_ReturnThruPtr_data((uint8_t *)data);

  mem_read_8_ExpectAndReturn(addr + 1, NULL, STATUS_OK);
  mem_read_8_IgnoreArg_data();
  mem_read_8_ReturnThruPtr_data(((uint8_t *)data) + 1);
}

void stub_mem_write_16(uint16_t addr, uint16_t data)
{
  mem_write_8_ExpectAndReturn(addr, (data & 0xFF), STATUS_OK);
  mem_write_8_ExpectAndReturn(addr + 1, (data >> 8) & 0xFF, STATUS_OK);
}