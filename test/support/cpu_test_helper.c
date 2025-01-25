#include "cpu_test_helper.h"

#include <stdint.h>

#include "unity.h"
#include "status_code.h"
#include "cpu.h"
#include "mock_bus_interface.h"
#include "mock_interrupt.h"

static interrupt_handle_t interrrupt_handle;

void stub_cpu_state_init(cpu_state_t *state)
{
  state->registers.pc = TEST_PC_INIT_VALUE;
  state->m_cycles = 0;
  state->run_mode = RUN_MODE_NORMAL;
};

void stub_mem_read_8(uint16_t addr, uint8_t *data)
{
  bus_interface_read_ExpectAndReturn(NULL, addr, NULL, STATUS_OK);
  bus_interface_read_IgnoreArg_bus_interface();
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data(data);
}

void stub_mem_read_16(uint16_t addr, uint16_t *data)
{
  bus_interface_read_ExpectAndReturn(NULL, addr, NULL, STATUS_OK);
  bus_interface_read_IgnoreArg_bus_interface();
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data((uint8_t *)data);

  bus_interface_read_ExpectAndReturn(NULL, addr + 1, NULL, STATUS_OK);
  bus_interface_read_IgnoreArg_bus_interface();
  bus_interface_read_IgnoreArg_data();
  bus_interface_read_ReturnThruPtr_data(((uint8_t *)data) + 1);
}

void stub_mem_write_16(uint16_t addr, uint16_t data)
{
  bus_interface_write_ExpectAndReturn(NULL, addr, (data & 0xFF), STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();
  bus_interface_write_ExpectAndReturn(NULL, addr + 1, (data >> 8) & 0xFF, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();
}