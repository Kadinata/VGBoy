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

void stub_test_ALU_REG_REG__core(uint8_t opcode, cpu_state_t *state, uint16_t *dest, uint16_t expected_result)
{
  stub_cpu_state_init(state);

  /* Read opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify output */
  TEST_ASSERT_EQUAL_HEX16(expected_result, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_ALU_REG_S8__core(uint8_t opcode, cpu_state_t *state, uint16_t *dest, int8_t s8_data, uint16_t expected_result)
{
  stub_cpu_state_init(state);

  /* Read opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read immediate S8 data */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, (uint8_t *)&s8_data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify output */
  TEST_ASSERT_EQUAL_HEX16(expected_result, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
}

void stub_test_ADD_HL_REG(uint8_t opcode, cpu_state_t *state, uint16_t *reg)
{
  /* No flag affected */
  *reg = 0x4321;
  state->registers.hl = 0x1234;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, &state->registers.hl, 0x5555);
  TEST_ASSERT_EQUAL_HEX16(0x4321, *reg);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, 0x00, state->registers.f);

  /* H flag only */
  *reg = 0x0FFF;
  state->registers.hl = 0x0001;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, &state->registers.hl, 0x1000);
  TEST_ASSERT_EQUAL_HEX16(0x0FFF, *reg);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_H, state->registers.f);

  /* H flag cleared */
  *reg = 0x0FFF;
  state->registers.hl = 0xF000;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, &state->registers.hl, 0xFFFF);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, 0x00, state->registers.f);

  /* C flags only */
  *reg = 0xF000;
  state->registers.hl = 0x1FFF;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, &state->registers.hl, 0x0FFF);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_C, state->registers.f);

  /* H and C flags */
  *reg = 0x0001;
  state->registers.hl = 0xFFFF;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, &state->registers.hl, 0x0000);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_H | FLAG_C, state->registers.f);
}

/**
 * 0x03: INC BC
 */
void test_INC_BC(void)
{
  cpu_state_t state = {0};
  state.registers.bc = 0x1234;
  stub_test_ALU_REG_REG__core(0x03, &state, &state.registers.bc, 0x1235);
}

/**
 * 0x09: ADD HL, BC
 */
void test_ADD_HL_BC(void)
{
  uint8_t opcode = 0x09;
  cpu_state_t state = {0};
  stub_test_ADD_HL_REG(opcode, &state, &state.registers.bc);
}

/**
 * 0x0B: DEC BC
 */
void test_DEC_BC(void)
{
  cpu_state_t state = {0};
  state.registers.bc = 0x1234;
  stub_test_ALU_REG_REG__core(0x0B, &state, &state.registers.bc, 0x1233);
}

/**
 * 0x13: INC DE
 */
void test_INC_DE(void)
{
  cpu_state_t state = {0};
  state.registers.de = 0x1234;
  stub_test_ALU_REG_REG__core(0x13, &state, &state.registers.de, 0x1235);
}

/**
 * 0x19: ADD HL, DE
 */
void test_ADD_HL_DE(void)
{
  uint8_t opcode = 0x19;
  cpu_state_t state = {0};
  stub_test_ADD_HL_REG(opcode, &state, &state.registers.de);
}

/**
 * 0x1B: DEC DE
 */
void test_DEC_DE(void)
{
  cpu_state_t state = {0};
  state.registers.de = 0x1234;
  stub_test_ALU_REG_REG__core(0x1B, &state, &state.registers.de, 0x1233);
}

/**
 * 0x23: INC HL
 */
void test_INC_HL(void)
{
  cpu_state_t state = {0};
  state.registers.hl = 0x1234;
  stub_test_ALU_REG_REG__core(0x23, &state, &state.registers.hl, 0x1235);
}

/**
 * 0x29: ADD HL, HL
 */
void test_ADD_HL_HL(void)
{
  cpu_state_t state = {0};

  /* No flag affected */
  state.registers.hl = 0x1234;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(0x29, &state, &state.registers.hl, 0x2468);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* H flag only */
  state.registers.hl = 0x0800;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x29, &state, &state.registers.hl, 0x1000);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_H, state.registers.f);

  /* H flag cleared */
  state.registers.hl = 0x1000;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(0x29, &state, &state.registers.hl, 0x2000);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* C flags only */
  state.registers.hl = 0x8123;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x29, &state, &state.registers.hl, 0x0246);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_C, state.registers.f);

  /* H and C flags */
  state.registers.hl = 0x8812;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x29, &state, &state.registers.hl, 0x1024);
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x2B: DEC HL
 */
void test_DEC_HL(void)
{
  cpu_state_t state = {0};
  state.registers.hl = 0x1234;
  stub_test_ALU_REG_REG__core(0x2B, &state, &state.registers.hl, 0x1233);
}

/**
 * 0x33: INC SP
 */
void test_INC_SP(void)
{
  cpu_state_t state = {0};
  state.registers.sp = 0x1234;
  stub_test_ALU_REG_REG__core(0x33, &state, &state.registers.sp, 0x1235);
}

/**
 * 0x39: ADD HL, SP
 */
void test_ADD_HL_SP(void)
{
  uint8_t opcode = 0x39;
  cpu_state_t state = {0};
  stub_test_ADD_HL_REG(opcode, &state, &state.registers.sp);
}

/**
 * 0x3B: DEC SP
 */
void test_DEC_SP(void)
{
  cpu_state_t state = {0};
  state.registers.sp = 0x1234;
  stub_test_ALU_REG_REG__core(0x3B, &state, &state.registers.sp, 0x1233);
}

/**
 * 0xE8: ADD SP, s8
 */
void test_ADD_SP_s8(void)
{
  cpu_state_t state = {0};

  /* No flag affected, positive offset */
  state.registers.sp = 0x1234;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, 0x43, 0x1277);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* No flag affected, negative offset */
  // state.registers.sp = 0x1234;
  // state.registers.f = 0xF0;
  // stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, -0x12, 0x1222);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* H flag only, positive offset */
  state.registers.sp = 0x000F;
  state.registers.f = 0x00;
  stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, 0x01, 0x0010);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H, state.registers.f);

  /* H flag only, negative offset */
  // state.registers.sp = 0x0FFF;
  // state.registers.f = 0x00;
  // stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, 0x01, 0x1000);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H, state.registers.f);

  /* H and C flags, positive offset */
  state.registers.sp = 0xFFFF;
  state.registers.f = 0x00;
  stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, 0x01, 0x0000);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H | FLAG_C, state.registers.f);

  /* H and C flags, negative offset */
  // state.registers.sp = 0xFFFF;
  // state.registers.f = 0x00;
  // stub_test_ALU_REG_S8__core(0xE8, &state, &state.registers.sp, 0x01, 0x0000);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_C, state.registers.f);
}