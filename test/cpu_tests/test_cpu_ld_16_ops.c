#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "cpu_test_helper.h"
#include "mock_memory.h"

TEST_FILE("cpu.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void stub_test_LD_REG_d16(uint8_t opcode, cpu_state_t *state, uint16_t *reg)
{
  uint16_t data = 0x1234;
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(data, *reg);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_POP_REG(uint8_t opcode, cpu_state_t *state, uint16_t *reg)
{
  uint16_t data = 0x1234;
  uint16_t initial_sp = 0x200;

  stub_cpu_state_init(state);
  state->registers.sp = initial_sp;
  *reg = 0x0000;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(initial_sp, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(data, *reg);
  TEST_ASSERT_EQUAL_HEX16(initial_sp + 2, state->registers.sp);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_PUSH_REG(uint8_t opcode, cpu_state_t *state, uint16_t *reg)
{
  uint16_t data = 0x1234;
  uint16_t initial_sp = 0x200;

  stub_cpu_state_init(state);
  state->registers.sp = initial_sp;
  *reg = data;

  /* Read the current opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Push the register's value onto the stack */
  stub_mem_write_16(initial_sp - 2, data);

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
}

void stub_test_LD_REG_s8(uint8_t opcode, cpu_state_t *state, uint16_t *dest, int8_t s8_data, uint16_t expected_result)
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
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

/**
 * 0x01: LD BC, d16
 */
void test_LD_BC_d16(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_d16(0x01, &state, &state.registers.bc);
}

/**
 * 0x08: LD [a16], SP
 */
void test_LD_a16_SP(void)
{
  uint8_t opcode = 0x08;
  uint16_t address = 0x1234;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.sp = 0x5678;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);
  stub_mem_write_16(address, 0x5678);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(5, state.m_cycles);
}

/**
 * 0x11: LD DE, d16
 */
void test_LD_DE_d16(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_d16(0x11, &state, &state.registers.de);
}

/**
 * 0x21: LD HL, d16
 */
void test_LD_HL_d16(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_d16(0x21, &state, &state.registers.hl);
}

/**
 * 0x31: LD SP, d16
 */
void test_LD_SP_d16(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_d16(0x31, &state, &state.registers.sp);
}

/**
 * 0xC1: POP BC
 */
void test_POP_BC(void)
{
  cpu_state_t state = {0};
  stub_test_POP_REG(0xC1, &state, &state.registers.bc);
}

/**
 * 0xC5: PUSH BC
 */
void test_PUSH_BC(void)
{
  cpu_state_t state = {0};
  stub_test_PUSH_REG(0xC5, &state, &state.registers.bc);
}

/**
 * 0xD1: POP DE
 */
void test_POP_DE(void)
{
  cpu_state_t state = {0};
  stub_test_POP_REG(0xD1, &state, &state.registers.de);
}

/**
 * 0xD5: PUSH DE
 */
void test_PUSH_DE(void)
{
  cpu_state_t state = {0};
  stub_test_PUSH_REG(0xD5, &state, &state.registers.de);
}

/**
 * 0xE1: POP HL
 */
void test_POP_HL(void)
{
  cpu_state_t state = {0};
  stub_test_POP_REG(0xE1, &state, &state.registers.hl);
}

/**
 * 0xE5: PUSH HL
 */
void test_PUSH_HL(void)
{
  cpu_state_t state = {0};
  stub_test_PUSH_REG(0xE5, &state, &state.registers.hl);
}

/**
 * 0xF1: POP AF
 */
void test_POP_AF(void)
{
  cpu_state_t state = {0};
  stub_test_POP_REG(0xF1, &state, &state.registers.af);
}

/**
 * 0xF5: PUSH AF
 */
void test_PUSH_AF(void)
{
  cpu_state_t state = {0};
  stub_test_PUSH_REG(0xF5, &state, &state.registers.af);
}

/**
 * 0xF8: LD HL, SP+s8
 */
void test_LD_HL_SP_s8(void)
{
  uint8_t opcode = 0xF8;
  cpu_state_t state = {0};

  /* No flag affected, positive offset */
  state.registers.hl = 0x00;
  state.registers.sp = 0x1234;
  state.registers.f = 0xF0;
  stub_test_LD_REG_s8(0xF8, &state, &state.registers.hl, 0x43, 0x1277);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* No flag affected, negative offset */
  // state.registers.sp = 0x1234;
  // state.registers.f = 0xF0;
  // stub_test_LD_REG_s8(0xE8, &state, &state.registers.sp, -0x12, 0x1222);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  /* H flag only, positive offset */
  state.registers.hl = 0x00;
  state.registers.sp = 0x000F;
  state.registers.f = 0x00;
  stub_test_LD_REG_s8(0xF8, &state, &state.registers.hl, 0x01, 0x0010);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H, state.registers.f);

  /* H flag only, negative offset */
  // state.registers.sp = 0x0FFF;
  // state.registers.f = 0x00;
  // stub_test_LD_REG_s8(0xF8, &state, &state.registers.sp, 0x01, 0x1000);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H, state.registers.f);

  /* H and C flags, positive offset */
  state.registers.hl = 0x00;
  state.registers.sp = 0xFFFF;
  state.registers.f = 0x00;
  stub_test_LD_REG_s8(0xF8, &state, &state.registers.hl, 0x01, 0x0000);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_H | FLAG_C, state.registers.f);

  /* H and C flags, negative offset */
  // state.registers.sp = 0xFFFF;
  // state.registers.f = 0x00;
  // stub_test_LD_REG_s8(0xF8, &state, &state.registers.hl, 0x01, 0x0000);
  // TEST_ASSERT_BITS(FLAG_Z | FLAG_N | FLAG_H | FLAG_C, FLAG_C, state.registers.f);
}

/**
 * 0xF9: LD SP, HL
 */
void test_LD_SP_HL(void)
{
  uint8_t opcode = 0xF9;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;
  state.registers.sp = 0x0000;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.sp);
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}