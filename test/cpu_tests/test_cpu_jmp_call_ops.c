#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "mock_bus_interface.h"
#include "mock_interrupt.h"
#include "mock_timing_sync.h"
#include "mock_debug_serial.h"
#include "cpu_test_helper.h"

#define TEST_RST(RST_NUM, RST_VECTOR, OPCODE)  \
  void test_RST_##RST_NUM(void)                \
  {                                            \
    cpu_state_t state = {0};                   \
    stub_test_RST(OPCODE, &state, RST_VECTOR); \
  }

TEST_FILE("cpu.c")

void setUp(void)
{
  serial_check_Ignore();
}

void tearDown(void)
{
}

void stub_test_JR_expect_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  int8_t offsets[] = {100, -100};

  for (int8_t i = 0; i < 2; i++)
  {
    stub_cpu_state_init(state);
    state->registers.f = flags;

    stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
    stub_mem_read_8(TEST_PC_INIT_VALUE + 1, (uint8_t *)&offsets[i]);

    TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
    TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2 + offsets[i], state->registers.pc);
    TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
  }
}

void stub_test_JR_expect_no_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  int8_t offset = 100;

  stub_cpu_state_init(state);
  state->registers.f = flags;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, (uint8_t *)&offset);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_JP_expect_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  uint16_t address = 0x1234;

  stub_cpu_state_init(state);
  state->registers.f = flags;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(address, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
}

void stub_test_JP_expect_no_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  uint16_t address = 0x1234;

  stub_cpu_state_init(state);
  state->registers.f = flags;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_CALL_expect_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  uint16_t address = 0x1234;
  uint16_t initial_sp = 0x200;

  stub_cpu_state_init(state);
  state->registers.f = flags;
  state->registers.sp = initial_sp;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read the next 2 bytes following opcode */
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  /* Push PC onto the stack */
  stub_mem_write_16(initial_sp - 2, TEST_PC_INIT_VALUE + 3);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* PC should point to the loaded address now */
  TEST_ASSERT_EQUAL_HEX16(address, state->registers.pc);

  TEST_ASSERT_EQUAL_INT(6, state->m_cycles);
}

void stub_test_CALL_expect_no_jump(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  uint16_t address = 0x1234;

  stub_cpu_state_init(state);
  state->registers.f = flags;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_RET_expect_return(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  uint16_t data = 0x1234;
  uint16_t initial_sp = 0x200;

  stub_cpu_state_init(state);
  state->registers.f = flags;
  state->registers.sp = initial_sp;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_16(initial_sp, &data);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(data, state->registers.pc);
  TEST_ASSERT_EQUAL_HEX16(initial_sp + 2, state->registers.sp);

  if (opcode == 0xC9)
  {
    /* 0xC9 is RET, which takes only 4 M-cycles */
    TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
  }
  else
  {
    TEST_ASSERT_EQUAL_INT(5, state->m_cycles);
  }
}

void stub_test_RET_expect_no_return(uint8_t opcode, cpu_state_t *state, uint8_t flags)
{
  stub_cpu_state_init(state);
  state->registers.f = flags;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_RST(uint8_t opcode, cpu_state_t *state, uint16_t reset_vector)
{
  uint16_t initial_sp = 0x200;

  stub_cpu_state_init(state);
  state->registers.sp = initial_sp;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Push PC onto the stack */
  stub_mem_write_16(initial_sp - 2, TEST_PC_INIT_VALUE + 1);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(reset_vector, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
}

/**
 * 0x18: JR s8
 */
void test_JR_s8(void)
{
  cpu_state_t state = {0};
  stub_test_JR_expect_jump(0x18, &state, 0);
}

/**
 * 0x20: JR NZ, s8
 */
void test_JR_NZ_s8(void)
{
  cpu_state_t state = {0};
  stub_test_JR_expect_jump(0x20, &state, (0xF0 & ~FLAG_Z)); // Z flag = 0
  stub_test_JR_expect_no_jump(0x20, &state, FLAG_Z);        // Z flag = 1
}

/**
 * 0x28: JR Z, s8
 */
void test_JR_Z_s8(void)
{
  cpu_state_t state = {0};
  stub_test_JR_expect_no_jump(0x28, &state, (0xF0 & ~FLAG_Z)); // Z flag = 0
  stub_test_JR_expect_jump(0x28, &state, FLAG_Z);              // Z flag = 1
}

/**
 * 0x30: JR NC, s8
 */
void test_JR_NC_s8(void)
{
  cpu_state_t state = {0};
  stub_test_JR_expect_jump(0x30, &state, (0xF0 & ~FLAG_C)); // C flag = 0
  stub_test_JR_expect_no_jump(0x30, &state, FLAG_C);        // C flag = 1
}

/**
 * 0x38: JR C, s8
 */
void test_JR_C_s8(void)
{
  cpu_state_t state = {0};
  stub_test_JR_expect_no_jump(0x38, &state, (0xF0 & ~FLAG_C)); // C flag = 0
  stub_test_JR_expect_jump(0x38, &state, FLAG_C);              // C flag = 1
}

/**
 * 0xC0: RET NZ
 */
void test_RET_NZ(void)
{
  cpu_state_t state = {0};
  stub_test_RET_expect_return(0xC0, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_RET_expect_no_return(0xC0, &state, FLAG_Z);                // Z flag = 1
}

/**
 * 0xC2: JP NZ, a16
 */
void test_JP_NZ_a16(void)
{
  cpu_state_t state = {0};
  stub_test_JP_expect_jump(0xC2, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_JP_expect_no_jump(0xC2, &state, FLAG_Z);                // Z flag = 1
}

/**
 * 0xC3: JP a16
 */
void test_JP_a16(void)
{
  cpu_state_t state = {0};
  stub_test_JP_expect_jump(0xC3, &state, 0x00);
}

/**
 * 0xC4: CALL NZ, a16
 */
void test_CALL_NZ_a16(void)
{
  cpu_state_t state = {0};
  stub_test_CALL_expect_jump(0xC4, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_CALL_expect_no_jump(0xC4, &state, FLAG_Z);                // Z flag = 1
}

/**
 * 0xC8: RET Z
 */
void test_RET_Z(void)
{
  cpu_state_t state = {0};
  stub_test_RET_expect_no_return(0xC8, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_RET_expect_return(0xC8, &state, FLAG_Z);                      // Z flag = 1
}

/**
 * 0xC9: RET
 */
void test_RET(void)
{
  cpu_state_t state = {0};
  stub_test_RET_expect_return(0xC9, &state, 0x00);
}

/**
 * 0xCA: JP Z, a16
 */
void test_JP_Z_a16(void)
{
  cpu_state_t state = {0};
  stub_test_JP_expect_no_jump(0xCA, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_JP_expect_jump(0xCA, &state, FLAG_Z);                      // Z flag = 1
}

/**
 * 0xCC: CALL Z, a16
 */
void test_CALL_Z_a16(void)
{
  cpu_state_t state = {0};
  stub_test_CALL_expect_no_jump(0xCC, &state, FLAG_N | FLAG_H | FLAG_C); // Z flag = 0
  stub_test_CALL_expect_jump(0xCC, &state, FLAG_Z);                      // Z flag = 1
}

/**
 * 0xCD: CALL, a16
 */
void test_CALL_a16(void)
{
  cpu_state_t state = {0};
  stub_test_CALL_expect_jump(0xCD, &state, 0x00);
}

/**
 * 0xD0: RET NC
 */
void test_RET_NC(void)
{
  cpu_state_t state = {0};
  stub_test_RET_expect_return(0xD0, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_RET_expect_no_return(0xD0, &state, FLAG_C);                // C flag = 1
}

/**
 * 0xD2: JP NC, a16
 */
void test_JP_NC_a16(void)
{
  cpu_state_t state = {0};
  stub_test_JP_expect_jump(0xD2, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_JP_expect_no_jump(0xD2, &state, FLAG_C);                // C flag = 1
}

/**
 * 0xD4: CALL NC, a16
 */
void test_CALL_NC_a16(void)
{
  cpu_state_t state = {0};
  stub_test_CALL_expect_jump(0xD4, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_CALL_expect_no_jump(0xD4, &state, FLAG_C);                // C flag = 1
}

/**
 * 0xD8: RET C
 */
void test_RET_C(void)
{
  cpu_state_t state = {0};
  stub_test_RET_expect_no_return(0xD8, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_RET_expect_return(0xD8, &state, FLAG_C);                      // C flag = 1
}

/**
 * 0xDA: JP C, a16
 */
void test_JP_C_a16(void)
{
  cpu_state_t state = {0};
  stub_test_JP_expect_no_jump(0xDA, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_JP_expect_jump(0xDA, &state, FLAG_C);                      // C flag = 1
}

/**
 * 0xDC: CALL C, a16
 */
void test_CALL_C_a16__C_flag_0(void)
{
  cpu_state_t state = {0};
  stub_test_CALL_expect_no_jump(0xDC, &state, FLAG_Z | FLAG_N | FLAG_H); // C flag = 0
  stub_test_CALL_expect_jump(0xDC, &state, FLAG_C);                      // C flag = 1
}

/**
 * 0xE9: JP HL
 */
void test_JP_HL(void)
{
  uint8_t opcode = 0xE9;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state.m_cycles);
}

/* RST N */
TEST_RST(0, 0x0000, 0xC7);
TEST_RST(1, 0x0008, 0xCF);
TEST_RST(2, 0x0010, 0xD7);
TEST_RST(3, 0x0018, 0xDF);
TEST_RST(4, 0x0020, 0xE7);
TEST_RST(5, 0x0028, 0xEF);
TEST_RST(6, 0x0030, 0xF7);
TEST_RST(7, 0x0038, 0xFF);
