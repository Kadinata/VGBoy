#include "unity.h"
#include "cpu.h"

#include "mock_bus_interface.h"
#include "mock_callback.h"
#include "mock_interrupt.h"
#include "mock_debug_serial.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_regs(void)
{
  cpu_state_t state = {0};

  state.registers.a = 0xAA;
  state.registers.b = 0xBB;
  state.registers.c = 0xCC;
  state.registers.d = 0xDD;
  state.registers.e = 0xEE;
  state.registers.f = 0xFF;
  state.registers.h = 0x11;
  state.registers.l = 0x22;

  TEST_ASSERT_EQUAL_HEX16(0xAAFF, state.registers.af);
  TEST_ASSERT_EQUAL_HEX16(0xBBCC, state.registers.bc);
  TEST_ASSERT_EQUAL_HEX16(0xDDEE, state.registers.de);
  TEST_ASSERT_EQUAL_HEX16(0x1122, state.registers.hl);
}