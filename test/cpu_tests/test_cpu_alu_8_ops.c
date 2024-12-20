#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "cpu_test_helper.h"

#include "mock_memory.h"

#define TEST_INC_REG(REG_NAME, REG, OPCODE)                  \
  void test_INC_##REG_NAME(void)                             \
  {                                                          \
    cpu_state_t state = {0};                                 \
    stub_test_INC_REG(OPCODE, &state, &state.registers.REG); \
  }

#define TEST_DEC_REG(REG_NAME, REG, OPCODE)                  \
  void test_DEC_##REG_NAME(void)                             \
  {                                                          \
    cpu_state_t state = {0};                                 \
    stub_test_DEC_REG(OPCODE, &state, &state.registers.REG); \
  }

TEST_FILE("cpu.c")

void setUp(void)
{
}

void tearDown(void)
{
}

void stub_test_ALU_REG_REG__core(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t expected_result)
{
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX8(expected_result, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state->m_cycles);
}

void stub_test_ALU_REG_MEM__core(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint16_t address, uint8_t data, uint8_t expected_sum)
{
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(address, &data);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX8(expected_sum, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_ALU_REG_D8__core(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t data, uint8_t expected_result)
{
  stub_cpu_state_init(state);

  /* Load the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Load immediate 8-bit data */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &data);

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify the execution output */
  TEST_ASSERT_EQUAL_HEX8(expected_result, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_INC_REG(uint8_t opcode, cpu_state_t *state, uint8_t *reg)
{
  /* No flags affected */
  *reg = 0xAA;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0xAA + 1);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, 0x00, state->registers.f);

  /* Z and H flags */
  *reg = 0xFF;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0x00);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_Z | FLAG_H, state->registers.f);

  /* H flag only */
  *reg = 0x0F;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0x10);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_H, state->registers.f);
}

void stub_test_DEC_REG(uint8_t opcode, cpu_state_t *state, uint8_t *reg)
{
  /* N flag only */
  *reg = 0xAA;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0xAA - 1);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_N, state->registers.f);

  /* Z and N flags */
  *reg = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0x00);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_Z | FLAG_N, state->registers.f);

  /* H and N flags */
  *reg = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, reg, 0x0F);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_H | FLAG_N, state->registers.f);
}

void stub_test_INC_MEM__core(uint8_t opcode, cpu_state_t *state, uint16_t address, uint8_t data)
{
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(address, &data);
  mem_write_8_ExpectAndReturn(address, data + 1, STATUS_OK);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_DEC_MEM__core(uint8_t opcode, cpu_state_t *state, uint16_t address, uint8_t data)
{
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(address, &data);
  mem_write_8_ExpectAndReturn(address, data - 1, STATUS_OK);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_ADD_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected */
  *dest = 0x32;
  *src = 0x23;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x55);
  TEST_ASSERT_EQUAL_HEX8(0x23, *src);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* Z Flag only */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state->registers.f);

  /* H Flag only */
  *dest = 0x0F;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state->registers.f);

  /* C Flag only */
  *dest = 0xF0;
  *src = 0x15;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state->registers.f);

  /* Z and C Flag only */
  *dest = 0xF0;
  *src = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state->registers.f);

  /* H and C Flag only */
  *dest = 0xF8;
  *src = 0x0F;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state->registers.f);

  /* All flags */
  *dest = 0xFF;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state->registers.f);
}

void stub_test_ADC_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected; no carry */
  *dest = 0x32;
  *src = 0x23;
  state->registers.f = FLAG_Z | FLAG_N | FLAG_H;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x55);
  TEST_ASSERT_EQUAL_HEX8(0x23, *src);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* No flags affected; with carry */
  *dest = 0x32;
  *src = 0x23;
  state->registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x56);
  TEST_ASSERT_EQUAL_HEX8(0x23, *src);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* Z flag only */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state->registers.f);

  /* no Z flag, with carry */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x01);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* H Flag only, no carry */
  *dest = 0x0F;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state->registers.f);

  /* H Flag only, with carry */
  *dest = 0x0E;
  *src = 0x01;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state->registers.f);

  /* C Flag only, no carry */
  *dest = 0xF0;
  *src = 0x15;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state->registers.f);

  /* C Flag only, with carry */
  *dest = 0xF0;
  *src = 0x10;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x01);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state->registers.f);

  /* Z and C Flag only, no carry */
  *dest = 0xF0;
  *src = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state->registers.f);

  /* H and C Flag only, no carry */
  *dest = 0xF8;
  *src = 0x0F;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state->registers.f);

  /* H and C Flag only, with carry */
  *dest = 0xF0;
  *src = 0x1F;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state->registers.f);

  /* All flags, no carry */
  *dest = 0xFF;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state->registers.f);

  /* All flags, with carry */
  *dest = 0xFF;
  *src = 0x00;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state->registers.f);
}

void stub_test_SUB_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected except for N flag */
  *dest = 0x76;
  *src = 0x54;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x22);
  TEST_ASSERT_EQUAL_HEX8(0x54, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state->registers.f);

  /* Z and N flags only test 1 */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* Z and N flags only test 2 */
  *dest = 0x55;
  *src = 0x55;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* H and N flags only */
  *dest = 0x10;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state->registers.f);

  /* C and N flags only */
  *dest = 0x00;
  *src = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state->registers.f);

  /* N, H, and C flags only */
  *dest = 0x00;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state->registers.f);
}

void stub_test_SBC_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected except for N flag, no carry */
  *dest = 0x76;
  *src = 0x54;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x22);
  TEST_ASSERT_EQUAL_HEX8(0x54, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state->registers.f);

  /* No flags affected except for N flag, with carry */
  *dest = 0x76;
  *src = 0x54;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x21);
  TEST_ASSERT_EQUAL_HEX8(0x54, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state->registers.f);

  /* Z and N flags only test 1, no carry */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* Z and N flags only test 1, with carry */
  *dest = 0x01;
  *src = 0x00;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* Z and N flags only test 2, no carry */
  *dest = 0x55;
  *src = 0x55;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* Z and N flags only test 2, with carry */
  *dest = 0x55;
  *src = 0x54;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* H and N flags only, no carry */
  *dest = 0x10;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state->registers.f);

  /* H and N flags only, with carry */
  *dest = 0x10;
  *src = 0x00;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state->registers.f);

  /* C and N flags only, no carry */
  *dest = 0x00;
  *src = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state->registers.f);

  /* C and N flags only, with carry */
  *dest = 0x01;
  *src = 0x10;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state->registers.f);

  /* N, H, and C flags only, no carry */
  *dest = 0x00;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state->registers.f);

  /* N, H, and C flags only, with carry */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state->registers.f);

  /* Z, N, and H flags with carry */
  *dest = 0x10;
  *src = 0x0F;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N | FLAG_H, state->registers.f);

  /* Carry edge case */
  *dest = 0xFF;
  *src = 0xFF;
  state->registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state->registers.f);
}

void stub_test_AND_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* H flags only */
  *dest = 0x53;
  *src = 0x35;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x11);
  TEST_ASSERT_EQUAL_HEX8(0x35, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state->registers.f);

  /* Z and H flags only */
  *dest = 0xAA;
  *src = 0x55;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H, state->registers.f);
}

void stub_test_XOR_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected */
  *dest = 0x53;
  *src = 0x35;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x66);
  TEST_ASSERT_EQUAL_HEX8(0x35, *src);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* Z flag only */
  *dest = 0x55;
  *src = 0x55;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state->registers.f);
}

void stub_test_OR_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected */
  *dest = 0x53;
  *src = 0x35;
  state->registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x77);
  TEST_ASSERT_EQUAL_HEX8(0x35, *src);
  TEST_ASSERT_BITS(0xF0, 0x00, state->registers.f);

  /* Z flag only */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state->registers.f);
}

void stub_test_CP_REG_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  /* No flags affected except for N flag */
  *dest = 0x76;
  *src = 0x54;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x76);
  TEST_ASSERT_EQUAL_HEX8(0x54, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state->registers.f);

  /* Z and N flags only test 1 */
  *dest = 0x00;
  *src = 0x00;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_EQUAL_HEX8(0x00, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* Z and N flags only test 2 */
  *dest = 0x55;
  *src = 0x55;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x55);
  TEST_ASSERT_EQUAL_HEX8(0x55, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state->registers.f);

  /* H and N flags only */
  *dest = 0x10;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x10);
  TEST_ASSERT_EQUAL_HEX8(0x01, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state->registers.f);

  /* C and N flags only */
  *dest = 0x00;
  *src = 0x10;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_EQUAL_HEX8(0x10, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state->registers.f);

  /* N, H, and C flags only */
  *dest = 0x00;
  *src = 0x01;
  state->registers.f = 0x00;
  stub_test_ALU_REG_REG__core(opcode, state, dest, 0x00);
  TEST_ASSERT_EQUAL_HEX8(0x01, *src);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state->registers.f);
}

/** INC REG */
TEST_INC_REG(B, b, 0x04);
TEST_INC_REG(C, c, 0x0C);
TEST_INC_REG(D, d, 0x14);
TEST_INC_REG(E, e, 0x1C);
TEST_INC_REG(H, h, 0x24);
TEST_INC_REG(L, l, 0x2C);
TEST_INC_REG(A, a, 0x3C);

/** DEC REG */
TEST_DEC_REG(B, b, 0x05);
TEST_DEC_REG(C, c, 0x0D);
TEST_DEC_REG(D, d, 0x15);
TEST_DEC_REG(E, e, 0x1D);
TEST_DEC_REG(H, h, 0x25);
TEST_DEC_REG(L, l, 0x2D);
TEST_DEC_REG(A, a, 0x3D);

/**
 * 0x2F: CPL
 */
void test_CPL(void)
{
  uint8_t opcode = 0x2F;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0xAA;
  state.registers.f = 0x00;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(0x55, state.registers.a);

  TEST_ASSERT_BITS_HIGH(FLAG_N, state.registers.f);
  TEST_ASSERT_BITS_HIGH(FLAG_H, state.registers.f);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/**
 * 0x34: INC [HL]
 */
void test_INC_MEM_HL(void)
{
  cpu_state_t state = {0};
  state.registers.hl = 0x1234;

  /* No flags affected */
  stub_test_INC_MEM__core(0x34, &state, 0x1234, 0xAA);
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, 0x00, state.registers.f);

  /* Z and H flags */
  stub_test_INC_MEM__core(0x34, &state, 0x1234, 0xFF);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_Z | FLAG_H, state.registers.f);

  /* H flag only */
  stub_test_INC_MEM__core(0x34, &state, 0x1234, 0x0F);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_H, state.registers.f);
}

/**
 * 0x35: DEC [HL]
 */
void test_DEC_MEM_HL(void)
{
  cpu_state_t state = {0};
  state.registers.hl = 0x1234;

  /* N flag only */
  stub_test_DEC_MEM__core(0x35, &state, 0x1234, 0xAA);
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_N, state.registers.f);

  /* Z and N flags */
  stub_test_DEC_MEM__core(0x35, &state, 0x1234, 0x01);
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags */
  stub_test_DEC_MEM__core(0x35, &state, 0x1234, 0x10);
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);
  TEST_ASSERT_BITS(FLAG_Z | FLAG_H | FLAG_N, FLAG_N | FLAG_H, state.registers.f);
}

/**
 * 0x37: SCF
 */
void test_SCF(void)
{
  uint8_t opcode = 0x37;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_C, state.registers.f);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/**
 * 0x3F: CCF
 */
void test_CCF__C_flag_0(void)
{
  uint8_t opcode = 0x3F;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, FLAG_C, state.registers.f);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/**
 * 0x3F: CCF
 */
void test_CCF__C_flag_1(void)
{
  uint8_t opcode = 0x3F;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_BITS(FLAG_N | FLAG_H | FLAG_C, 0x00, state.registers.f);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/**
 * 0x80: ADD A, B
 */
void test_ADD_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x80, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0x81: ADD A, C
 */
void test_ADD_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x81, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0x82: ADD A, D
 */
void test_ADD_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x82, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0x83: ADD A, E
 */
void test_ADD_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x83, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0x84: ADD A, H
 */
void test_ADD_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x84, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0x85: ADD A, L
 */
void test_ADD_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_ADD_REG_REG(0x85, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0x86: ADD A, [HL]
 */
void test_ADD_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x32;
  state.registers.hl = 0x1234;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x23, 0x55);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* H flag only */
  state.registers.a = 0x0F;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C flag only */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x15, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C flags only */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C flags only */
  state.registers.a = 0xF8;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x0F, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* All flags */
  state.registers.a = 0xFF;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x86, &state, &state.registers.a, 0x1234, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x87: ADD A, A
 */
void test_ADD_A_A(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x32;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x64);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* H flag only */
  state.registers.a = 0x08;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C flag only */
  state.registers.a = 0x81;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x02);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C flags only */
  state.registers.a = 0x80;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C flags only */
  state.registers.a = 0x88;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x87, &state, &state.registers.a, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x88: ADC A, B
 */
void test_ADC_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x88, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0x89: ADC A, C
 */
void test_ADC_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x89, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0x8A: ADC A, D
 */
void test_ADC_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x8A, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0x8B: ADC A, E
 */
void test_ADC_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x8B, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0x8C: ADC A, H
 */
void test_ADC_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x8C, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0x8D: ADC A, L
 */
void test_ADC_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_ADC_REG_REG(0x8D, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0x8E: ADC A, [HL]
 */
void test_ADC_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected; no carry */
  state.registers.a = 0x32;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x23, 0x55);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* No flags affected; with carry */
  state.registers.a = 0x32;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x23, 0x56);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* no Z flag, with carry */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x00, 0x01);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* H Flag only, no carry */
  state.registers.a = 0x0F;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* H Flag only, with carry */
  state.registers.a = 0x0E;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C Flag only, no carry */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x15, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* C Flag only, with carry */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x10, 0x01);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C Flag only, no carry */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C Flag only, no carry */
  state.registers.a = 0xF8;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x0F, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* H and C Flag only, with carry */
  state.registers.a = 0xF0;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x1F, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* All flags, no carry */
  state.registers.a = 0xFF;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);

  /* All flags, with carry */
  state.registers.a = 0xFF;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x8E, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x8F: ADC A, A
 */
void test_ADC_A_A(void)
{
  cpu_state_t state = {0};

  /* No flags affected, no carry */
  state.registers.a = 0x32;
  state.registers.f = FLAG_Z | FLAG_H | FLAG_N;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x64);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* No flags affected, with carry */
  state.registers.a = 0x32;
  state.registers.f = FLAG_Z | FLAG_H | FLAG_N | FLAG_C;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x65);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only, no carry */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* no Z flag, with carry */
  state.registers.a = 0x00;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x01);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* H flag only, no carry */
  state.registers.a = 0x08;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* H flag only, with carry */
  state.registers.a = 0x08;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x11);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C flag only, no carry */
  state.registers.a = 0x81;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x02);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* C flag only, with carry */
  state.registers.a = 0x80;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x01);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C flags only, no carry */
  state.registers.a = 0x80;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C flags only, no carry */
  state.registers.a = 0x88;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* H and C flags only, with carry */
  state.registers.a = 0x88;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_REG__core(0x8F, &state, &state.registers.a, 0x11);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x90: SUB A, B
 */
void test_SUB_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x90, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0x91: SUB A, C
 */
void test_SUB_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x91, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0x92: SUB A, D
 */
void test_SUB_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x92, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0x93: SUB A, E
 */
void test_SUB_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x93, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0x94: SUB A, H
 */
void test_SUB_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x94, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0x95: SUB A, L
 */
void test_SUB_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_SUB_REG_REG(0x95, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0x96: SUB A, [HL]
 */
void test_SUB_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x76;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x54, 0x22);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z flag only test 1 */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z flag only test 2 */
  state.registers.a = 0x55;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only */
  state.registers.a = 0x10;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x01, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x96, &state, &state.registers.a, 0x1234, 0x01, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x97: SUB A, A
 */
void test_SUB_A_A(void)
{
  cpu_state_t state = {0};

  for (uint16_t data = 0x00; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = 0x00;
    stub_test_ALU_REG_REG__core(0x97, &state, &state.registers.a, 0x00);
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);
  }
}

/**
 * 0x98: SBC A, B
 */
void test_SBC_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x98, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0x99: SBC A, C
 */
void test_SBC_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x99, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0x9A: SBC A, D
 */
void test_SBC_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x9A, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0x9B: SBC A, E
 */
void test_SBC_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x9B, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0x9C: SBC A, H
 */
void test_SBC_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x9C, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0x9D: SBC A, L
 */
void test_SBC_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_SBC_REG_REG(0x9D, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0x9E: SBC A, [HL]
 */
void test_SBC_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected except for N flag, no carry */
  state.registers.a = 0x76;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x54, 0x22);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* No flags affected except for N flag, with carry */
  state.registers.a = 0x76;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x54, 0x21);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z and N flags only test 1, no carry */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 1, with carry */
  state.registers.a = 0x01;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 2, no carry */
  state.registers.a = 0x55;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 2, with carry */
  state.registers.a = 0x55;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x54, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only, no carry */
  state.registers.a = 0x10;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x01, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* H and N flags only, with carry */
  state.registers.a = 0x10;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x00, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only, no carry */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* C and N flags only, with carry */
  state.registers.a = 0x01;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only, no carry */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x01, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);

  /* N, H, and C flags only, with carry */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x00, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);

  /* Z, N, and H flags with carry */
  state.registers.a = 0x10;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0x0F, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N | FLAG_H, state.registers.f);

  /* Carry edge case */
  state.registers.a = 0xFF;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_MEM__core(0x9E, &state, &state.registers.a, 0x1234, 0xFF, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0x9F: SBC A, A
 */
void test_SBC_A_A__no_carry(void)
{
  cpu_state_t state = {0};

  for (uint16_t data = 0x00; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = 0x00;
    stub_test_ALU_REG_REG__core(0x9F, &state, &state.registers.a, 0x00);
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);
  }
}

/**
 * 0x9F: SBC A, A
 */
void test_SBC_A_A__with_carry(void)
{
  cpu_state_t state = {0};

  /** With carry */
  for (uint16_t data = 0x00; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = FLAG_C;
    stub_test_ALU_REG_REG__core(0x9F, &state, &state.registers.a, 0xFF);
    TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
  }
}

/**
 * 0xA0: AND A, B
 */
void test_AND_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA0, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0xA1: AND A, C
 */
void test_AND_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA1, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0xA2: AND A, D
 */
void test_AND_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA2, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0xA3: AND A, E
 */
void test_AND_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA3, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0xA4: AND A, H
 */
void test_AND_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA4, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0xA5: AND A, L
 */
void test_AND_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_AND_REG_REG(0xA5, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0xA6: AND A, [HL]
 */
void test_AND_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* H flags only */
  state.registers.a = 0x53;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_MEM__core(0xA6, &state, &state.registers.a, 0x1234, 0x35, 0x11);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* Z and H flags only */
  state.registers.a = 0xAA;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xA6, &state, &state.registers.a, 0x1234, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H, state.registers.f);
}

/**
 * 0xA7: AND A, A
 */
void test_AND_A_A(void)
{
  cpu_state_t state = {0};

  /* H flags only */
  state.registers.a = 0xAA;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_REG__core(0xA7, &state, &state.registers.a, 0xAA);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* Z and H flags only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_REG__core(0xA7, &state, &state.registers.a, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H, state.registers.f);
}

/**
 * 0xA8: XOR A, B
 */
void test_XOR_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xA8, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0xA9: XOR A, C
 */
void test_XOR_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xA9, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0xAA: XOR A, D
 */
void test_XOR_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xAA, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0xAB: XOR A, E
 */
void test_XOR_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xAB, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0xAC: XOR A, H
 */
void test_XOR_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xAC, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0xAD: XOR A, L
 */
void test_XOR_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_XOR_REG_REG(0xAD, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0xAE: XOR A, [HL]
 */
void test_XOR_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x53;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_MEM__core(0xAE, &state, &state.registers.a, 0x1234, 0x35, 0x66);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0xAA;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xAE, &state, &state.registers.a, 0x1234, 0xAA, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);
}

/**
 * 0xAF: XOR A, A
 */
void test_XOR_A_A(void)
{
  cpu_state_t state = {0};

  for (uint16_t data = 0; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = 0x00;
    stub_test_ALU_REG_REG__core(0xAF, &state, &state.registers.a, 0x00);
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);
  }
}

/**
 * 0xB0: OR A, B
 */
void test_OR_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB0, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0xB1: OR A, C
 */
void test_OR_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB1, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0xB2: OR A, D
 */
void test_OR_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB2, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0xB3: OR A, E
 */
void test_OR_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB3, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0xB4: OR A, H
 */
void test_OR_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB4, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0xB5: OR A, L
 */
void test_OR_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_OR_REG_REG(0xB5, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0xB6: OR A, [HL]
 */
void test_OR_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x53;
  state.registers.hl = 0x1234;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_MEM__core(0xB6, &state, &state.registers.a, 0x1234, 0x35, 0x77);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xB6, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);
}

/**
 * 0xB7: OR A, A
 */
void test_OR_A_A(void)
{
  cpu_state_t state = {0};

  for (uint16_t data = 0; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = 0x00;
    stub_test_ALU_REG_REG__core(0xB7, &state, &state.registers.a, data & 0xFF);
    TEST_ASSERT_BITS(0xF0, (data ? 0x00 : FLAG_Z), state.registers.f);
  }
}

/**
 * 0xB8: CP A, B
 */
void test_CP_A_B(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xB8, &state, &state.registers.a, &state.registers.b);
}

/**
 * 0xB9: CP A, C
 */
void test_CP_A_C(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xB9, &state, &state.registers.a, &state.registers.c);
}

/**
 * 0xBA: CP A, D
 */
void test_CP_A_D(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xBA, &state, &state.registers.a, &state.registers.d);
}

/**
 * 0xBB: CP A, E
 */
void test_CP_A_E(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xBB, &state, &state.registers.a, &state.registers.e);
}

/**
 * 0xBC: CP A, H
 */
void test_CP_A_H(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xBC, &state, &state.registers.a, &state.registers.h);
}

/**
 * 0xBD: CP A, L
 */
void test_CP_A_L(void)
{
  cpu_state_t state = {0};
  stub_test_CP_REG_REG(0xBD, &state, &state.registers.a, &state.registers.l);
}

/**
 * 0xBE: CP A, [HL]
 */
void test_CP_A_MEM_HL(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x76;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x54, 0x76);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z flag only test 1 */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z flag only test 2 */
  state.registers.a = 0x55;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x55, 0x55);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only */
  state.registers.a = 0x10;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only */
  state.registers.a = 0x00;
  state.registers.hl = 0x1234;
  state.registers.f = 0x00;
  stub_test_ALU_REG_MEM__core(0xBE, &state, &state.registers.a, 0x1234, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0xBF: CP A, A
 */
void test_CP_A_A(void)
{
  cpu_state_t state = {0};

  for (uint16_t data = 0x00; data < 0x100; data++)
  {
    state.registers.a = data & 0xFF;
    state.registers.f = 0x00;
    stub_test_ALU_REG_REG__core(0xBF, &state, &state.registers.a, data);
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);
  }
}

/**
 * 0xC6: ADD A, d8
 */
void test_ADD_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x32;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x23, 0x55);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* H flag only */
  state.registers.a = 0x0F;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C flag only */
  state.registers.a = 0xF0;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x15, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C flags only */
  state.registers.a = 0xF0;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C flags only */
  state.registers.a = 0xF8;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x0F, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* All flags */
  state.registers.a = 0xFF;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xC6, &state, &state.registers.a, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0xCE: ADC A, d8
 */
void test_ADC_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flags affected; no carry */
  state.registers.a = 0x32;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x23, 0x55);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* No flags affected; with carry */
  state.registers.a = 0x32;
  state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x23, 0x56);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);

  /* no Z flag, with carry */
  state.registers.a = 0x00;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x00, 0x01);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* H Flag only, no carry */
  state.registers.a = 0x0F;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* H Flag only, with carry */
  state.registers.a = 0x0E;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* C Flag only, no carry */
  state.registers.a = 0xF0;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x15, 0x05);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* C Flag only, with carry */
  state.registers.a = 0xF0;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x10, 0x01);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);

  /* Z and C Flag only, no carry */
  state.registers.a = 0xF0;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);

  /* H and C Flag only, no carry */
  state.registers.a = 0xF8;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x0F, 0x07);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* H and C Flag only, with carry */
  state.registers.a = 0xF0;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x1F, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_C, state.registers.f);

  /* All flags, no carry */
  state.registers.a = 0xFF;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);

  /* All flags, with carry */
  state.registers.a = 0xFF;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xCE, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0xD6: SUB A, d8
 */
void test_SUB_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x76;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x54, 0x22);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z flag only test 1 */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z flag only test 2 */
  state.registers.a = 0x55;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only */
  state.registers.a = 0x10;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x01, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xD6, &state, &state.registers.a, 0x01, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0xDE: SBC A, d8
 */
void test_SBC_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flags affected except for N flag, no carry */
  state.registers.a = 0x76;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x54, 0x22);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* No flags affected except for N flag, with carry */
  state.registers.a = 0x76;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x54, 0x21);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z and N flags only test 1, no carry */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 1, with carry */
  state.registers.a = 0x01;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 2, no carry */
  state.registers.a = 0x55;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z and N flags only test 2, with carry */
  state.registers.a = 0x55;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x54, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only, no carry */
  state.registers.a = 0x10;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x01, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* H and N flags only, with carry */
  state.registers.a = 0x10;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x00, 0x0F);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only, no carry */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* C and N flags only, with carry */
  state.registers.a = 0x01;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x10, 0xF0);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only, no carry */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x01, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);

  /* N, H, and C flags only, with carry */
  state.registers.a = 0x00;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x00, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);

  /* Z, N, and H flags with carry */
  state.registers.a = 0x10;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0x0F, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N | FLAG_H, state.registers.f);

  /* Carry edge case */
  state.registers.a = 0xFF;
  state.registers.f = FLAG_C;
  stub_test_ALU_REG_D8__core(0xDE, &state, &state.registers.a, 0xFF, 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}

/**
 * 0xE6: AND A, d8
 */
void test_AND_A_d8(void)
{
  cpu_state_t state = {0};

  /* H flags only */
  state.registers.a = 0x53;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xE6, &state, &state.registers.a, 0x35, 0x11);
  TEST_ASSERT_BITS(0xF0, FLAG_H, state.registers.f);

  /* Z and H flags only */
  state.registers.a = 0xAA;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xE6, &state, &state.registers.a, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_H, state.registers.f);
}

/**
 * 0xEE: XOR A, d8
 */
void test_XOR_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flag affected */
  state.registers.a = 0x53;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xEE, &state, &state.registers.a, 0x35, 0x66);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x55;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xEE, &state, &state.registers.a, 0x55, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);
}

/**
 * 0xF6: OR A, d8
 */
void test_OR_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flag affected */
  state.registers.a = 0x53;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xF6, &state, &state.registers.a, 0x35, 0x77);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* Z flag only */
  state.registers.a = 0x00;
  state.registers.f = 0xF0;
  stub_test_ALU_REG_D8__core(0xF6, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);
}

/**
 * 0xFE: CP A, d8
 */
void test_CP_A_d8(void)
{
  cpu_state_t state = {0};

  /* No flags affected */
  state.registers.a = 0x76;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x54, 0x76);
  TEST_ASSERT_BITS(0xF0, FLAG_N, state.registers.f);

  /* Z flag only test 1 */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x00, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* Z flag only test 2 */
  state.registers.a = 0x55;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x55, 0x55);
  TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_N, state.registers.f);

  /* H and N flags only */
  state.registers.a = 0x10;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x01, 0x10);
  TEST_ASSERT_BITS(0xF0, FLAG_H | FLAG_N, state.registers.f);

  /* C and N flags only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x10, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_C, state.registers.f);

  /* N, H, and C flags only */
  state.registers.a = 0x00;
  state.registers.f = 0x00;
  stub_test_ALU_REG_D8__core(0xFE, &state, &state.registers.a, 0x01, 0x00);
  TEST_ASSERT_BITS(0xF0, FLAG_N | FLAG_H | FLAG_C, state.registers.f);
}