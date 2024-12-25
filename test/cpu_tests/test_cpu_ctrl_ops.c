#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "mock_bus_interface.h"
#include "mock_interrupt.h"
#include "mock_timing_sync.h"
#include "mock_debug_serial.h"
#include "cpu_test_helper.h"

#define TEST_PC_INIT_VALUE (0x100)

TEST_FILE("cpu.c")

void setUp(void)
{
  serial_check_Ignore();
}

void tearDown(void)
{
}

/**
 * 0x00: NOP
 */
void test_NOP(void)
{
  uint8_t opcode = 0x00;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}


/**
 * 0x10: NOP
 */
void test_STOP(void)
{
  uint8_t opcode = 0x10;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_INT(RUN_MODE_STOPPED, state.run_mode);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/**
 * 0x76: HALT
 */
void test_HALT(void)
{
  uint8_t opcode = 0x76;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_INT(RUN_MODE_HALTED, state.run_mode);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}