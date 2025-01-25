#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "mock_bus_interface.h"
#include "mock_callback.h"
#include "mock_interrupt.h"
#include "mock_debug_serial.h"
#include "cpu_test_helper.h"

#define TEST_LD_REG_REG(DEST_NAME, DEST_REG, SRC_NAME, SRC_REG, OPCODE)                          \
  void test_LD_##DEST_NAME##_##SRC_NAME(void)                                                    \
  {                                                                                              \
    cpu_state_t state = {0};                                                                     \
    stub_test_LD_REG_2_REG(OPCODE, &state, &state.registers.DEST_REG, &state.registers.SRC_REG); \
  }

#define TEST_LD_REG_MEM_HL(DEST_NAME, DEST_REG, OPCODE)                                     \
  void test_LD_##DEST_NAME##_MEM_HL(void)                                                   \
  {                                                                                         \
    cpu_state_t state = {0};                                                                \
    stub_test_LD_MEM_2_REG(OPCODE, &state, &state.registers.DEST_REG, &state.registers.hl); \
  }

#define TEST_LD_MEM_HL_REG(SRC_NAME, SRC_REG, OPCODE)                                      \
  void test_LD_MEM_HL_##SRC_NAME(void)                                                     \
  {                                                                                        \
    cpu_state_t state = {0};                                                               \
    stub_test_LD_REG_2_MEM(OPCODE, &state, &state.registers.hl, &state.registers.SRC_REG); \
  }

TEST_FILE("cpu.c")

void setUp(void)
{
  serial_check_Ignore();
}

void tearDown(void)
{
}

void stub_test_LD_REG_IMM_8(uint8_t opcode, cpu_state_t *state, uint8_t *reg)
{
  uint8_t data = 0xAA;

  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX16(data, *reg);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_LD_REG_2_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint8_t *src)
{
  stub_cpu_state_init(state);
  *dest = 0x00;
  *src = 0xAA;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));
  TEST_ASSERT_EQUAL_HEX8(0xAA, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state->m_cycles);
}

void stub_test_LD_REG_2_MEM(uint8_t opcode, cpu_state_t *state, uint16_t *dest, uint8_t *src)
{
  uint16_t addr = 0x1234;
  uint8_t data = 0xAA;

  stub_cpu_state_init(state);
  *src = data;
  *dest = addr;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  bus_interface_write_ExpectAndReturn(NULL, addr, data, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_LD_MEM_2_REG(uint8_t opcode, cpu_state_t *state, uint8_t *dest, uint16_t *src)
{
  uint16_t addr = 0x1234;
  uint8_t data = 0xAA;

  stub_cpu_state_init(state);
  *src = addr;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(addr, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX8(data, *dest);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

/**
 * 0x02: LD [BC], A
 */
void test_LD_BC_A(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_2_MEM(0x02, &state, &state.registers.bc, &state.registers.a);
}

/**
 * 0x06: LD B, d8
 */
void test_LD_B_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x06, &state, &state.registers.b);
}

/**
 * 0x0A: LD A, [BC]
 */
void test_LD_A_MEM_BC(void)
{
  cpu_state_t state = {0};
  stub_test_LD_MEM_2_REG(0x0A, &state, &state.registers.a, &state.registers.bc);
}

/**
 * 0x0E: LD C, d8
 */
void test_LD_C_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x0E, &state, &state.registers.c);
}

/**
 * 0x12: LD [DE], A
 */
void test_LD_MEM_DE_A(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_2_MEM(0x12, &state, &state.registers.de, &state.registers.a);
}

/**
 * 0x16: LD D, d8
 */
void test_LD_D_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x16, &state, &state.registers.d);
}

/**
 * 0x1A: LD A, [DE]
 */
void test_LD_A_MEM_DE(void)
{
  cpu_state_t state = {0};
  stub_test_LD_MEM_2_REG(0x1A, &state, &state.registers.a, &state.registers.de);
}

/**
 * 0x1E: LD E, d8
 */
void test_LD_E_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x1E, &state, &state.registers.e);
}

/**
 * 0x22: LD [HL+], A
 */
void test_LD_HL_INC_A(void)
{
  uint8_t opcode = 0x22;
  cpu_state_t state = {0};
  stub_cpu_state_init(&state);

  state.registers.a = 0xAA;
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  bus_interface_write_ExpectAndReturn(NULL, 0x1234, 0xAA, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(0x1235, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0x26: LD H, d8
 */
void test_LD_H_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x26, &state, &state.registers.h);
}

/**
 * 0x2A: LD A, [HL+]
 */
void test_LD_A_HL_INC(void)
{
  uint8_t opcode = 0x2A;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(0x1234, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(data, state.registers.a);
  TEST_ASSERT_EQUAL_HEX16(0x1235, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0x2E: LD L, d8
 */
void test_LD_L_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x2E, &state, &state.registers.l);
}

/**
 * 0x32: LD [HL-], A
 */
void test_LD_HL_DEC_A(void)
{
  uint8_t opcode = 0x32;
  cpu_state_t state = {0};
  stub_cpu_state_init(&state);

  state.registers.a = 0xAA;
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  bus_interface_write_ExpectAndReturn(NULL, 0x1234, 0xAA, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(0x1233, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0x36: LD [HL], d8
 */
void test_LD_HL_d8(void)
{
  uint8_t opcode = 0x36;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &data);
  bus_interface_write_ExpectAndReturn(NULL, 0x1234, data, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX16(0x1234, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state.m_cycles);
}

/**
 * 0x3A: LD A, [HL-]
 */
void test_LD_A_HL_DEC(void)
{
  uint8_t opcode = 0x3A;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  stub_mem_read_8(0x1234, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(data, state.registers.a);
  TEST_ASSERT_EQUAL_HEX16(0x1233, state.registers.hl);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0x3E: LD A, d8
 */
void test_LD_A_d8(void)
{
  cpu_state_t state = {0};
  stub_test_LD_REG_IMM_8(0x3E, &state, &state.registers.a);
}

/* 0x40 - 0x47: LD B, X */
TEST_LD_REG_REG(B, b, B, b, 0x40);
TEST_LD_REG_REG(B, b, C, c, 0x41);
TEST_LD_REG_REG(B, b, D, d, 0x42);
TEST_LD_REG_REG(B, b, E, e, 0x43);
TEST_LD_REG_REG(B, b, H, h, 0x44);
TEST_LD_REG_REG(B, b, L, l, 0x45);
TEST_LD_REG_MEM_HL(B, b, 0x46);
TEST_LD_REG_REG(B, b, A, a, 0x47);

/* 0x48 - 0x4F: LD C, X */
TEST_LD_REG_REG(C, c, B, b, 0x48);
TEST_LD_REG_REG(C, c, C, c, 0x49);
TEST_LD_REG_REG(C, c, D, d, 0x4A);
TEST_LD_REG_REG(C, c, E, e, 0x4B);
TEST_LD_REG_REG(C, c, H, h, 0x4C);
TEST_LD_REG_REG(C, c, L, l, 0x4D);
TEST_LD_REG_MEM_HL(C, c, 0x4E);
TEST_LD_REG_REG(C, c, A, a, 0x4F);

/* 0x50 - 0x57: LD D, X */
TEST_LD_REG_REG(D, d, B, b, 0x50);
TEST_LD_REG_REG(D, d, C, c, 0x51);
TEST_LD_REG_REG(D, d, D, d, 0x52);
TEST_LD_REG_REG(D, d, E, e, 0x53);
TEST_LD_REG_REG(D, d, H, h, 0x54);
TEST_LD_REG_REG(D, d, L, l, 0x55);
TEST_LD_REG_MEM_HL(D, d, 0x56);
TEST_LD_REG_REG(D, d, A, a, 0x57);

/* 0x58 - 0x5F: LD E, X */
TEST_LD_REG_REG(E, e, B, b, 0x58);
TEST_LD_REG_REG(E, e, C, c, 0x59);
TEST_LD_REG_REG(E, e, D, d, 0x5A);
TEST_LD_REG_REG(E, e, E, e, 0x5B);
TEST_LD_REG_REG(E, e, H, h, 0x5C);
TEST_LD_REG_REG(E, e, L, l, 0x5D);
TEST_LD_REG_MEM_HL(E, e, 0x5E);
TEST_LD_REG_REG(E, e, A, a, 0x5F);

/* 0x60 - 0x67: LD H, X */
TEST_LD_REG_REG(H, h, B, b, 0x60);
TEST_LD_REG_REG(H, h, C, c, 0x61);
TEST_LD_REG_REG(H, h, D, d, 0x62);
TEST_LD_REG_REG(H, h, E, e, 0x63);
TEST_LD_REG_REG(H, h, H, h, 0x64);
TEST_LD_REG_REG(H, h, L, l, 0x65);
TEST_LD_REG_MEM_HL(H, h, 0x66);
TEST_LD_REG_REG(H, h, A, a, 0x67);

/* 0x68 - 0x6F: LD L, X */
TEST_LD_REG_REG(L, l, B, b, 0x68);
TEST_LD_REG_REG(L, l, C, c, 0x69);
TEST_LD_REG_REG(L, l, D, d, 0x6A);
TEST_LD_REG_REG(L, l, E, e, 0x6B);
TEST_LD_REG_REG(L, l, H, h, 0x6C);
TEST_LD_REG_REG(L, l, L, l, 0x6D);
TEST_LD_REG_MEM_HL(L, l, 0x6E);
TEST_LD_REG_REG(L, l, A, a, 0x6F);

/* 0x70 - 0x75, 0x77: LD [HL], REG */
TEST_LD_MEM_HL_REG(B, b, 0x70);
TEST_LD_MEM_HL_REG(C, c, 0x71);
TEST_LD_MEM_HL_REG(D, d, 0x72);
TEST_LD_MEM_HL_REG(E, e, 0x73);

/**
 * 0x74: LD [HL], H
 */
void test_LD_MEM_HL_H(void)
{
  uint8_t opcode = 0x74;
  cpu_state_t state = {0};
  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  bus_interface_write_ExpectAndReturn(NULL, 0x1234, 0x12, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0x75: LD [HL], L
 */
void test_LD_MEM_HL_L(void)
{
  uint8_t opcode = 0x75;
  cpu_state_t state = {0};
  stub_cpu_state_init(&state);
  state.registers.hl = 0x1234;

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  bus_interface_write_ExpectAndReturn(NULL, 0x1234, 0x34, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

TEST_LD_MEM_HL_REG(A, a, 0x77);

/* 0x79 - 0x7F: LD A, X */
TEST_LD_REG_REG(A, a, B, b, 0x78);
TEST_LD_REG_REG(A, a, C, c, 0x79);
TEST_LD_REG_REG(A, a, D, d, 0x7A);
TEST_LD_REG_REG(A, a, E, e, 0x7B);
TEST_LD_REG_REG(A, a, H, h, 0x7C);
TEST_LD_REG_REG(A, a, L, l, 0x7D);
TEST_LD_REG_MEM_HL(A, a, 0x7E);
TEST_LD_REG_REG(A, a, A, a, 0x7F);

/**
 * 0xE0: LD [a8], A
 */
void test_LD_IMM_A8_A(void)
{
  uint8_t opcode = 0xE0;
  uint8_t offset = 0x55;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0xAA;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read immediate A8 offset */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &offset);

  /* Should write the content of register A to 0xFF00 + offset */
  bus_interface_write_ExpectAndReturn(NULL, 0xFF00 | offset, 0xAA, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state.m_cycles);
}

/**
 * 0xE2: LD [C], A
 */
void test_LD_MEM_C_A(void)
{
  uint8_t opcode = 0xE2;
  uint8_t offset = 0x55;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0xAA;
  state.registers.c = offset;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Should write the content of register A to 0xFF00 + register C value */
  bus_interface_write_ExpectAndReturn(NULL, 0xFF00 | offset, 0xAA, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0xEA: LD [a16], A
 */
void test_LD_IMM_a16_A(void)
{
  uint16_t address = 0x1234;
  uint8_t opcode = 0xEA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0xAA;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read immediate A16 address */
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  /* Store the value of Register A in the memory location specified by the address */
  bus_interface_write_ExpectAndReturn(NULL, address, 0xAA, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state.m_cycles);
}

/**
 * 0xF0: LD A, [a8]
 */
void test_LD_A_IMM_A8(void)
{
  uint8_t opcode = 0xF0;
  uint8_t offset = 0x55;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read immediate A8 offset */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &offset);

  /* Should read from 0xFF00 + offset */
  stub_mem_read_8(0xFF00 | offset, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(data, state.registers.a);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state.m_cycles);
}

/**
 * 0xF2: LD A, [C]
 */
void test_LD_A_MEM_C(void)
{
  uint8_t opcode = 0xF2;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0x00;
  state.registers.c = 0x55;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read from 0xFF00 + register C */
  stub_mem_read_8(0xFF00 | state.registers.c, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(data, state.registers.a);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state.m_cycles);
}

/**
 * 0xFA: LD A, [a16]
 */
void test_LD_A_IMM_a16(void)
{
  uint8_t opcode = 0xFA;
  uint16_t address = 0x1234;
  uint8_t data = 0xAA;
  cpu_state_t state = {0};

  stub_cpu_state_init(&state);
  state.registers.a = 0x00;

  /* Read the opcode */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);

  /* Read immediate A16 address */
  stub_mem_read_16(TEST_PC_INIT_VALUE + 1, &address);

  /* Read from memory location specified by the address */
  stub_mem_read_8(address, &data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(&state));
  TEST_ASSERT_EQUAL_HEX8(data, state.registers.a);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 3, state.registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state.m_cycles);
}