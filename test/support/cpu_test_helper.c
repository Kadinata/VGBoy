#include "cpu_test_helper.h"

#include <stdint.h>

#include "unity.h"
#include "status_code.h"
#include "sys_def.h"
#include "mock_memory.h"

void stub_cpu_state_init(cpu_state_t *state)
{
  state->registers.pc = TEST_PC_INIT_VALUE;
  state->m_cycles = 0;
  state->run_mode = RUN_MODE_NORMAL;
};

void stub_mem_read_8(uint16_t addr, uint8_t *data)
{
  mem_read_8_ExpectAndReturn(addr, NULL, STATUS_OK);
  mem_read_8_IgnoreArg_data();
  mem_read_8_ReturnThruPtr_data(data);
}

void stub_mem_read_16(uint16_t addr, uint16_t *data)
{
  mem_read_16_ExpectAndReturn(addr, NULL, STATUS_OK);
  mem_read_16_IgnoreArg_data();
  mem_read_16_ReturnThruPtr_data(data);
}