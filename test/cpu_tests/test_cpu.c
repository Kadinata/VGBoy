#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "mock_bus_interface.h"
#include "mock_interrupt.h"
#include "mock_timing_sync.h"
#include "mock_debug_serial.h"
#include "cpu_test_helper.h"

TEST_FILE("cpu.c")

void setUp(void)
{
  serial_check_Ignore();
}

void tearDown(void)
{
}

void test_cpu_emulation_cycle_null_ptr(void)
{
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, cpu_emulation_cycle(NULL));
}

void test_cpu_undefined_opcodes(void)
{
  uint8_t undefined_opcodes[] = {
      0xD3, 0xDB, 0xDD, 0xE3,
      0xE4, 0xEB, 0xEC, 0xED,
      0xF4, 0xFC, 0xFD};

  cpu_state_t state = {0};

  for (int8_t i = 0; i < sizeof(undefined_opcodes); i++)
  {
    stub_cpu_state_init(&state);
    stub_mem_read_8(TEST_PC_INIT_VALUE, &undefined_opcodes[i]);
    TEST_ASSERT_EQUAL_INT(STATUS_ERR_UNDEFINED_INST, cpu_emulation_cycle(&state));
  }
}