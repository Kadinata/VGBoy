#include "unity.h"
#include "cpu.h"
#include "status_code.h"

#include "mock_bus_interface.h"
#include "mock_callback.h"
#include "mock_interrupt.h"
#include "mock_debug_serial.h"
#include "cpu_test_helper.h"

#define TEST_RLC_REG(REG_NAME, REG, OPCODE)                              \
  void test_RLC_##REG_NAME(void)                                         \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected, lsb 0 */                                        \
    state.registers.REG = 0x3C;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x78); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only */                                                    \
    state.registers.REG = 0xC3;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x87); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
  }

#define TEST_RLC_MEM_HL(OPCODE)                             \
  void test_RLC_MEM_HL(void)                                \
  {                                                         \
    cpu_state_t state = {0};                                \
                                                            \
    /* No flag affected, lsb 0 */                           \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x3C, 0x78); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);        \
                                                            \
    /* C flag only*/                                        \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xC3, 0x87); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);      \
                                                            \
    /* Z flag only */                                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);      \
  }

#define TEST_RRC_REG(REG_NAME, REG, OPCODE)                              \
  void test_RRC_##REG_NAME(void)                                         \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected, lsb 0 */                                        \
    state.registers.REG = 0x3C;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x1E); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only*/                                                     \
    state.registers.REG = 0xC3;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xE1); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
  }

#define TEST_RRC_MEM_HL(OPCODE)                             \
  void test_RRC_MEM_HL(void)                                \
  {                                                         \
    cpu_state_t state = {0};                                \
                                                            \
    /* No flag affected, lsb 0 */                           \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x3C, 0x1E); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);        \
                                                            \
    /* C flag only*/                                        \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xC3, 0xE1); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);      \
                                                            \
    /* Z flag only */                                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;           \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);      \
  }

#define TEST_RL_REG(REG_NAME, REG, OPCODE)                               \
  void test_RL_##REG_NAME(void)                                          \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected, no carry # 1 */                                 \
    state.registers.REG = 0x55;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xAA); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* No flag affected, with carry #1 */                                \
    state.registers.REG = 0x55;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xAB); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* No flag affected, with carry  #2*/                                \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x01); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only, no carry */                                          \
    state.registers.REG = 0xAA;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x54); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* C flag only, with carry */                                        \
    state.registers.REG = 0xAA;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x55); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z and C flags */                                                  \
    state.registers.REG = 0x80;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);          \
  }

#define TEST_RL_MEM_HL(OPCODE)                                  \
  void test_RL_MEM_HL(void)                                     \
  {                                                             \
    cpu_state_t state = {0};                                    \
                                                                \
    /* No flag affected, no carry # 1 */                        \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0xAA);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* No flag affected, with carry #1 */                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0xAB);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* No flag affected, with carry  #2*/                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x01);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* C flag only, no carry */                                 \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xAA, 0x54);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* C flag only, with carry */                               \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xAA, 0x55);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* Z flag only */                                           \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);          \
                                                                \
    /* Z and C flags */                                         \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x80, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f); \
  }

#define TEST_RR_REG(REG_NAME, REG, OPCODE)                               \
  void test_RR_##REG_NAME(void)                                          \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected, no carry # 1 */                                 \
    state.registers.REG = 0xAA;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x55); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* No flag affected, with carry #1 */                                \
    state.registers.REG = 0xAA;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xD5); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* No flag affected, with carry  #2*/                                \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x80); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only, no carry */                                          \
    state.registers.REG = 0x55;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x2A); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* C flag only, with carry */                                        \
    state.registers.REG = 0x55;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;               \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xAA); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z and C flags */                                                  \
    state.registers.REG = 0x01;                                          \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;                        \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);          \
  }

#define TEST_RR_MEM_HL(OPCODE)                                  \
  void test_RR_MEM_HL(void)                                     \
  {                                                             \
    cpu_state_t state = {0};                                    \
                                                                \
    /* No flag affected, no carry # 1 */                        \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xAA, 0x55);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* No flag affected, with carry #1 */                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xAA, 0xD5);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* No flag affected, with carry  #2*/                       \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x80);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* C flag only, no carry */                                 \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0x2A);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* C flag only, with carry */                               \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H | FLAG_C;      \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0xAA);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* Z flag only */                                           \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);          \
                                                                \
    /* Z and C flags */                                         \
    state.registers.f = FLAG_Z | FLAG_N | FLAG_H;               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x01, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f); \
  }

#define TEST_SLA_REG(REG_NAME, REG, OPCODE)                              \
  void test_SLA_##REG_NAME(void)                                         \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected */                                               \
    state.registers.REG = 0x55;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xAA); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only */                                                    \
    state.registers.REG = 0xA5;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x4A); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z and C flags */                                                  \
    state.registers.REG = 0x80;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);          \
  }

#define TEST_SLA_MEM_HL(OPCODE)                                 \
  void test_SLA_MEM_HL(void)                                    \
  {                                                             \
    cpu_state_t state = {0};                                    \
                                                                \
    /* No flag affected */                                      \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0xAA);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* C flag only */                                           \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xA5, 0x4A);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* Z flag only */                                           \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);          \
                                                                \
    /* Z and C flags */                                         \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x80, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f); \
  }

#define TEST_SRA_REG(REG_NAME, REG, OPCODE)                              \
  void test_SRA_##REG_NAME(void)                                         \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected, msb 0 */                                        \
    state.registers.REG = 0x5A;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x2D); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* No flag affected, msb 1 */                                        \
    state.registers.REG = 0xAA;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xD5); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only, msb 0 */                                             \
    state.registers.REG = 0x55;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x2A); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* C flag only, msb 1 */                                             \
    state.registers.REG = 0xA5;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xD2); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z and C flags */                                                  \
    state.registers.REG = 0x01;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);          \
  }

#define TEST_SRA_MEM_HL(OPCODE)                                 \
  void test_SRA_MEM_HL(void)                                    \
  {                                                             \
    cpu_state_t state = {0};                                    \
                                                                \
    /* No flag affected, msb 0 */                               \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x5A, 0x2D);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* No flag affected, msb 1 */                               \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xAA, 0xD5);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* C flag only, msb 0 */                                    \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x55, 0x2A);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* C flag only, msb 1 */                                    \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xA5, 0xD2);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* Z flag only */                                           \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);          \
                                                                \
    /* Z and C flags */                                         \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x01, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f); \
  }

#define TEST_SWAP_REG(REG_NAME, REG, OPCODE)                             \
  void test_SWAP_##REG_NAME(void)                                        \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected */                                               \
    state.registers.REG = 0x5A;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0xA5); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* Z flag only #1 */                                                 \
    state.registers.REG = 0x00;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z flag only #2 */                                                 \
    state.registers.REG = 0x00;                                          \
    state.registers.f = 0x00;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
  }

#define TEST_SWAP_MEM_HL(OPCODE)                            \
  void test_SWAP_MEM_HL(void)                               \
  {                                                         \
    cpu_state_t state = {0};                                \
                                                            \
    /* No flag affected */                                  \
    state.registers.f = 0xF0;                               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x5A, 0xA5); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);        \
                                                            \
    /* Z flag only #1 */                                    \
    state.registers.f = 0xF0;                               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);      \
                                                            \
    /* Z flag only #2 */                                    \
    state.registers.f = 0x00;                               \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);      \
  }

#define TEST_SRL_REG(REG_NAME, REG, OPCODE)                              \
  void test_SRL_##REG_NAME(void)                                         \
  {                                                                      \
    cpu_state_t state = {0};                                             \
                                                                         \
    /* No flag affected */                                               \
    state.registers.REG = 0x5A;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x2D); \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);                     \
                                                                         \
    /* C flag only */                                                    \
    state.registers.REG = 0xA5;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x52); \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);                   \
                                                                         \
    /* Z flag only */                                                    \
    state.registers.REG = 0x00;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);                   \
                                                                         \
    /* Z and C flags */                                                  \
    state.registers.REG = 0x01;                                          \
    state.registers.f = 0xF0;                                            \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, 0x00); \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f);          \
  }

#define TEST_SRL_MEM_HL(OPCODE)                                 \
  void test_SRL_MEM_HL(void)                                    \
  {                                                             \
    cpu_state_t state = {0};                                    \
                                                                \
    /* No flag affected */                                      \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x5A, 0x2D);     \
    TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);            \
                                                                \
    /* C flag only */                                           \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xA5, 0x52);     \
    TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);          \
                                                                \
    /* Z flag only */                                           \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z, state.registers.f);          \
                                                                \
    /* Z and C flags */                                         \
    state.registers.f = 0xF0;                                   \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x01, 0x00);     \
    TEST_ASSERT_BITS(0xF0, FLAG_Z | FLAG_C, state.registers.f); \
  }

#define TEST_BIT_REG(REG_NAME, REG, BIT_INDEX, OPCODE)            \
  void test_BIT_##BIT_INDEX##_##REG_NAME(void)                    \
  {                                                               \
    cpu_state_t state = {0};                                      \
                                                                  \
    /* test bit == 0 */                                           \
    state.registers.REG = (~(1 << BIT_INDEX)) & 0xFF;             \
    state.registers.f = 0x00;                                     \
    stub_test_CB_opcode_BIT_REG(OPCODE, &state, FLAG_Z | FLAG_H); \
                                                                  \
    /* test bit == 1 */                                           \
    state.registers.REG = (1 << BIT_INDEX);                       \
    state.registers.f = 0xF0;                                     \
    stub_test_CB_opcode_BIT_REG(OPCODE, &state, FLAG_H);          \
  }

#define TEST_BIT_MEM_HL(BIT_INDEX, OPCODE)                                                       \
  void test_BIT_##BIT_INDEX##_MEM_HL(void)                                                       \
  {                                                                                              \
    cpu_state_t state = {0};                                                                     \
                                                                                                 \
    /* test bit == 0 */                                                                          \
    state.registers.f = 0x00;                                                                    \
    stub_test_CB_opcode_BIT_MEM_HL(OPCODE, &state, (~(1 << BIT_INDEX)) & 0xFF, FLAG_Z | FLAG_H); \
                                                                                                 \
    /* test bit == 1 */                                                                          \
    state.registers.f = 0x00;                                                                    \
    stub_test_CB_opcode_BIT_MEM_HL(OPCODE, &state, (1 << BIT_INDEX), FLAG_H);                    \
  }

#define TEST_RST_REG(REG_NAME, REG, BIT_INDEX, OPCODE)                       \
  void test_RST_##BIT_INDEX##_##REG_NAME(void)                               \
  {                                                                          \
    cpu_state_t state = {0};                                                 \
    state.registers.REG = 0xFF;                                              \
    uint8_t expected = (~(1 << BIT_INDEX)) & 0xFF;                           \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, expected); \
  }

#define TEST_RST_MEM_HL(BIT_INDEX, OPCODE)                      \
  void test_RST_##BIT_INDEX##_MEM_HL(void)                      \
  {                                                             \
    cpu_state_t state = {0};                                    \
    uint8_t expected = (~(1 << BIT_INDEX)) & 0xFF;              \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0xFF, expected); \
  }

#define TEST_SET_REG(REG_NAME, REG, BIT_INDEX, OPCODE)                               \
  void test_SET_##BIT_INDEX##_##REG_NAME(void)                                       \
  {                                                                                  \
    cpu_state_t state = {0};                                                         \
    state.registers.REG = 0x00;                                                      \
    stub_test_CB_opcode_REG(OPCODE, &state, &state.registers.REG, (1 << BIT_INDEX)); \
  }

#define TEST_SET_MEM_HL(BIT_INDEX, OPCODE)                              \
  void test_SET_##BIT_INDEX##_MEM_HL(void)                              \
  {                                                                     \
    cpu_state_t state = {0};                                            \
    stub_test_CB_opcode_MEM_HL(OPCODE, &state, 0x00, (1 << BIT_INDEX)); \
  }

TEST_FILE("cpu.c")

void setUp(void)
{
  serial_check_Ignore();
}

void tearDown(void)
{
}

void stub_test_CB_opcode_REG(uint8_t opcode, cpu_state_t *state, uint8_t *reg, uint8_t expected_result)
{
  uint8_t opcode_prefix = 0xCB;

  stub_cpu_state_init(state);

  /* Read the first opcode (0xCB) */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode_prefix);

  /* Read the second opcode byte */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &opcode);

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify the result */
  TEST_ASSERT_EQUAL_HEX8(expected_result, *reg);

  /* Verrify cycle and instruction lengths */
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_CB_opcode_BIT_REG(uint8_t opcode, cpu_state_t *state, uint8_t expected_flag)
{
  uint8_t opcode_prefix = 0xCB;

  stub_cpu_state_init(state);

  /* Read the first opcode (0xCB) */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode_prefix);

  /* Read the second opcode byte */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &opcode);

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify the result */
  TEST_ASSERT_BITS(0xE0, expected_flag, state->registers.f);

  /* Verrify cycle and instruction lengths */
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(2, state->m_cycles);
}

void stub_test_CB_opcode_MEM_HL(uint8_t opcode, cpu_state_t *state, uint8_t data, uint8_t expected_result)
{
  uint8_t opcode_prefix = 0xCB;
  uint16_t address = 0x1234;

  stub_cpu_state_init(state);
  state->registers.hl = address;

  /* Read the first opcode (0xCB) */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode_prefix);

  /* Read the second opcode byte */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &opcode);

  /* Expect to read data from memory location pointed by register HL */
  stub_mem_read_8(address, &data);

  /* Expect to write the result back to memory location pointed by register HL */
  bus_interface_write_ExpectAndReturn(NULL, address, expected_result, STATUS_OK);
  bus_interface_write_IgnoreArg_bus_interface();

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify the value of register HL is unchanged */
  TEST_ASSERT_EQUAL_HEX16(address, state->registers.hl);

  /* Verrify cycle and instruction lengths */
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(4, state->m_cycles);
}

void stub_test_CB_opcode_BIT_MEM_HL(uint8_t opcode, cpu_state_t *state, uint8_t data, uint8_t expected_flag)
{
  uint8_t opcode_prefix = 0xCB;
  uint16_t address = 0x1234;

  stub_cpu_state_init(state);
  state->registers.hl = address;

  /* Read the first opcode (0xCB) */
  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode_prefix);

  /* Read the second opcode byte */
  stub_mem_read_8(TEST_PC_INIT_VALUE + 1, &opcode);

  /* Expect to read data from memory location pointed by register HL */
  stub_mem_read_8(address, &data);

  /* Execute the instruction */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  /* Verify the result */
  TEST_ASSERT_BITS(0xF0, expected_flag, state->registers.f);

  /* Verify the value of register HL is unchanged */
  TEST_ASSERT_EQUAL_HEX16(address, state->registers.hl);

  /* Verrify cycle and instruction lengths */
  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 2, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(3, state->m_cycles);
}

void stub_test_REG_A_rot(uint8_t opcode, cpu_state_t *state, uint8_t expected_value)
{
  stub_cpu_state_init(state);

  stub_mem_read_8(TEST_PC_INIT_VALUE, &opcode);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, cpu_emulation_cycle(state));

  TEST_ASSERT_EQUAL_HEX8(expected_value, state->registers.a);

  TEST_ASSERT_EQUAL_HEX16(TEST_PC_INIT_VALUE + 1, state->registers.pc);
  TEST_ASSERT_EQUAL_INT(1, state->m_cycles);
}

/**
 * 0x07: RLCA
 */
void test_RLCA(void)
{
  cpu_state_t state = {0};

  /* msb 0 */
  state.registers.a = 0x55;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x07, &state, (0x55 << 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* msb 1 */
  state.registers.a = 0xAA;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x07, &state, ((0xAA << 1) | 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);
}

/**
 * 0x0F: RLCA
 */
void test_RRCA(void)
{
  cpu_state_t state = {0};

  /* lsb 0 */
  state.registers.a = 0xAA;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x0F, &state, (0xAA >> 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* lsb 1 */
  state.registers.a = 0x55;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x0F, &state, ((0x55 >> 1) | (1 << 7)) & 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);
}

/**
 * 0x17: RLA
 */
void test_RLA(void)
{
  cpu_state_t state = {0};

  /* msb 0 with carry */
  state.registers.a = 0x5A;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x17, &state, ((0x5A << 1) | 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* msb 1 without carry */
  state.registers.a = 0xA5;
  state.registers.f = 0xF0 & (~FLAG_C);
  stub_test_REG_A_rot(0x17, &state, (0xA5 << 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);
}

/**
 * 0x1F: RRA
 */
void test_RRA(void)
{
  uint8_t opcode = 0x1F;
  cpu_state_t state = {0};

  /* lsb 0 with carry */
  state.registers.a = 0x5A;
  state.registers.f = 0xF0;
  stub_test_REG_A_rot(0x1F, &state, ((1 << 7) | (0x5A >> 1)) & 0xFF);
  TEST_ASSERT_BITS(0xF0, 0x00, state.registers.f);

  /* lsb 1 without carry */
  state.registers.a = 0xA5;
  state.registers.f = 0xF0 & (~FLAG_C);
  stub_test_REG_A_rot(0x1F, &state, (0xA5 >> 1) & 0xFF);
  TEST_ASSERT_BITS(0xF0, FLAG_C, state.registers.f);
}

/* 0xCB00 - 0xCB07: RLC REG */
TEST_RLC_REG(B, b, 0x00);
TEST_RLC_REG(C, c, 0x01);
TEST_RLC_REG(D, d, 0x02);
TEST_RLC_REG(E, e, 0x03);
TEST_RLC_REG(H, h, 0x04);
TEST_RLC_REG(L, l, 0x05);
TEST_RLC_MEM_HL(0x06);
TEST_RLC_REG(A, a, 0x07);

/* 0xCB08 - 0xCB0F: RRC REG */
TEST_RRC_REG(B, b, 0x08);
TEST_RRC_REG(C, c, 0x09);
TEST_RRC_REG(D, d, 0x0A);
TEST_RRC_REG(E, e, 0x0B);
TEST_RRC_REG(H, h, 0x0C);
TEST_RRC_REG(L, l, 0x0D);
TEST_RRC_MEM_HL(0x0E);
TEST_RRC_REG(A, a, 0x0F);

/* 0xCB10 - 0xCB17: RL REG */
TEST_RL_REG(B, b, 0x10);
TEST_RL_REG(C, c, 0x11);
TEST_RL_REG(D, d, 0x12);
TEST_RL_REG(E, e, 0x13);
TEST_RL_REG(H, h, 0x14);
TEST_RL_REG(L, l, 0x15);
TEST_RL_MEM_HL(0x16);
TEST_RL_REG(A, a, 0x17);

/* 0xCB18 - 0xCB1F: RR REG */
TEST_RR_REG(B, b, 0x18);
TEST_RR_REG(C, c, 0x19);
TEST_RR_REG(D, d, 0x1A);
TEST_RR_REG(E, e, 0x1B);
TEST_RR_REG(H, h, 0x1C);
TEST_RR_REG(L, l, 0x1D);
TEST_RR_MEM_HL(0x1E);
TEST_RR_REG(A, a, 0x1F);

/* 0xCB20 - 0xCB27: SLA REG */
TEST_SLA_REG(B, b, 0x20);
TEST_SLA_REG(C, c, 0x21);
TEST_SLA_REG(D, d, 0x22);
TEST_SLA_REG(E, e, 0x23);
TEST_SLA_REG(H, h, 0x24);
TEST_SLA_REG(L, l, 0x25);
TEST_SLA_MEM_HL(0x26);
TEST_SLA_REG(A, a, 0x27);

/* 0xCB28 - 0xCB2F: SRA REG */
TEST_SRA_REG(B, b, 0x28);
TEST_SRA_REG(C, c, 0x29);
TEST_SRA_REG(D, d, 0x2A);
TEST_SRA_REG(E, e, 0x2B);
TEST_SRA_REG(H, h, 0x2C);
TEST_SRA_REG(L, l, 0x2D);
TEST_SRA_MEM_HL(0x2E);
TEST_SRA_REG(A, a, 0x2F);

/* 0xCB30 - 0xCB37: SWAP REG */
TEST_SWAP_REG(B, b, 0x30);
TEST_SWAP_REG(C, c, 0x31);
TEST_SWAP_REG(D, d, 0x32);
TEST_SWAP_REG(E, e, 0x33);
TEST_SWAP_REG(H, h, 0x34);
TEST_SWAP_REG(L, l, 0x35);
TEST_SWAP_MEM_HL(0x36);
TEST_SWAP_REG(A, a, 0x37);

/* 0xCB38 - 0xCB3F: SRL REG */
TEST_SRL_REG(B, b, 0x38);
TEST_SRL_REG(C, c, 0x39);
TEST_SRL_REG(D, d, 0x3A);
TEST_SRL_REG(E, e, 0x3B);
TEST_SRL_REG(H, h, 0x3C);
TEST_SRL_REG(L, l, 0x3D);
TEST_SRL_MEM_HL(0x3E);
TEST_SRL_REG(A, a, 0x3F);

/** 0xCB40 - 0xCB47: BIT 0, REG */
TEST_BIT_REG(B, b, 0, 0x40);
TEST_BIT_REG(C, c, 0, 0x41);
TEST_BIT_REG(D, d, 0, 0x42);
TEST_BIT_REG(E, e, 0, 0x43);
TEST_BIT_REG(H, h, 0, 0x44);
TEST_BIT_REG(L, l, 0, 0x45);
TEST_BIT_MEM_HL(0, 0x46);
TEST_BIT_REG(A, a, 0, 0x47);

/** 0xCB48 - 0xCB4F: BIT 1, REG */
TEST_BIT_REG(B, b, 1, 0x48);
TEST_BIT_REG(C, c, 1, 0x49);
TEST_BIT_REG(D, d, 1, 0x4A);
TEST_BIT_REG(E, e, 1, 0x4B);
TEST_BIT_REG(H, h, 1, 0x4C);
TEST_BIT_REG(L, l, 1, 0x4D);
TEST_BIT_MEM_HL(1, 0x4E);
TEST_BIT_REG(A, a, 1, 0x4F);

/** 0xCB50 - 0xCB57: BIT 2, REG */
TEST_BIT_REG(B, b, 2, 0x50);
TEST_BIT_REG(C, c, 2, 0x51);
TEST_BIT_REG(D, d, 2, 0x52);
TEST_BIT_REG(E, e, 2, 0x53);
TEST_BIT_REG(H, h, 2, 0x54);
TEST_BIT_REG(L, l, 2, 0x55);
TEST_BIT_MEM_HL(2, 0x56);
TEST_BIT_REG(A, a, 2, 0x57);

/** 0xCB58 - 0xCB5F: BIT 3, REG */
TEST_BIT_REG(B, b, 3, 0x58);
TEST_BIT_REG(C, c, 3, 0x59);
TEST_BIT_REG(D, d, 3, 0x5A);
TEST_BIT_REG(E, e, 3, 0x5B);
TEST_BIT_REG(H, h, 3, 0x5C);
TEST_BIT_REG(L, l, 3, 0x5D);
TEST_BIT_MEM_HL(3, 0x5E);
TEST_BIT_REG(A, a, 3, 0x5F);

/** 0xCB60 - 0xCB67: BIT 4, REG */
TEST_BIT_REG(B, b, 4, 0x60);
TEST_BIT_REG(C, c, 4, 0x61);
TEST_BIT_REG(D, d, 4, 0x62);
TEST_BIT_REG(E, e, 4, 0x63);
TEST_BIT_REG(H, h, 4, 0x64);
TEST_BIT_REG(L, l, 4, 0x65);
TEST_BIT_MEM_HL(4, 0x66);
TEST_BIT_REG(A, a, 4, 0x67);

/** 0xCB68 - 0xCB6F: BIT 5, REG */
TEST_BIT_REG(B, b, 5, 0x68);
TEST_BIT_REG(C, c, 5, 0x69);
TEST_BIT_REG(D, d, 5, 0x6A);
TEST_BIT_REG(E, e, 5, 0x6B);
TEST_BIT_REG(H, h, 5, 0x6C);
TEST_BIT_REG(L, l, 5, 0x6D);
TEST_BIT_MEM_HL(5, 0x6E);
TEST_BIT_REG(A, a, 5, 0x6F);

/** 0xCB70 - 0xCB77: BIT 6, REG */
TEST_BIT_REG(B, b, 6, 0x70);
TEST_BIT_REG(C, c, 6, 0x71);
TEST_BIT_REG(D, d, 6, 0x72);
TEST_BIT_REG(E, e, 6, 0x73);
TEST_BIT_REG(H, h, 6, 0x74);
TEST_BIT_REG(L, l, 6, 0x75);
TEST_BIT_MEM_HL(6, 0x76);
TEST_BIT_REG(A, a, 6, 0x77);

/** 0xCB78 - 0xC7F: BIT 7, REG */
TEST_BIT_REG(B, b, 7, 0x78);
TEST_BIT_REG(C, c, 7, 0x79);
TEST_BIT_REG(D, d, 7, 0x7A);
TEST_BIT_REG(E, e, 7, 0x7B);
TEST_BIT_REG(H, h, 7, 0x7C);
TEST_BIT_REG(L, l, 7, 0x7D);
TEST_BIT_MEM_HL(7, 0x7E);
TEST_BIT_REG(A, a, 7, 0x7F);

/** 0xCB80 - 0xCB87: RST 0, REG */
TEST_RST_REG(B, b, 0, 0x80);
TEST_RST_REG(C, c, 0, 0x81);
TEST_RST_REG(D, d, 0, 0x82);
TEST_RST_REG(E, e, 0, 0x83);
TEST_RST_REG(H, h, 0, 0x84);
TEST_RST_REG(L, l, 0, 0x85);
TEST_RST_MEM_HL(0, 0x86);
TEST_RST_REG(A, a, 0, 0x87);

/** 0xCB88 - 0xCB8F: RST 1, REG */
TEST_RST_REG(B, b, 1, 0x88);
TEST_RST_REG(C, c, 1, 0x89);
TEST_RST_REG(D, d, 1, 0x8A);
TEST_RST_REG(E, e, 1, 0x8B);
TEST_RST_REG(H, h, 1, 0x8C);
TEST_RST_REG(L, l, 1, 0x8D);
TEST_RST_MEM_HL(1, 0x8E);
TEST_RST_REG(A, a, 1, 0x8F);

/** 0xCB90 - 0xCB97: RST 2, REG */
TEST_RST_REG(B, b, 2, 0x90);
TEST_RST_REG(C, c, 2, 0x91);
TEST_RST_REG(D, d, 2, 0x92);
TEST_RST_REG(E, e, 2, 0x93);
TEST_RST_REG(H, h, 2, 0x94);
TEST_RST_REG(L, l, 2, 0x95);
TEST_RST_MEM_HL(2, 0x96);
TEST_RST_REG(A, a, 2, 0x97);

/** 0xCB98 - 0xCB9F: RST 3, REG */
TEST_RST_REG(B, b, 3, 0x98);
TEST_RST_REG(C, c, 3, 0x99);
TEST_RST_REG(D, d, 3, 0x9A);
TEST_RST_REG(E, e, 3, 0x9B);
TEST_RST_REG(H, h, 3, 0x9C);
TEST_RST_REG(L, l, 3, 0x9D);
TEST_RST_MEM_HL(3, 0x9E);
TEST_RST_REG(A, a, 3, 0x9F);

/** 0xCBA0 - 0xCBA7: RST 4, REG */
TEST_RST_REG(B, b, 4, 0xA0);
TEST_RST_REG(C, c, 4, 0xA1);
TEST_RST_REG(D, d, 4, 0xA2);
TEST_RST_REG(E, e, 4, 0xA3);
TEST_RST_REG(H, h, 4, 0xA4);
TEST_RST_REG(L, l, 4, 0xA5);
TEST_RST_MEM_HL(4, 0xA6);
TEST_RST_REG(A, a, 4, 0xA7);

/** 0xCA8 - 0xCBAF: RST 5, REG */
TEST_RST_REG(B, b, 5, 0xA8);
TEST_RST_REG(C, c, 5, 0xA9);
TEST_RST_REG(D, d, 5, 0xAA);
TEST_RST_REG(E, e, 5, 0xAB);
TEST_RST_REG(H, h, 5, 0xAC);
TEST_RST_REG(L, l, 5, 0xAD);
TEST_RST_MEM_HL(5, 0xAE);
TEST_RST_REG(A, a, 5, 0xAF);

/** 0xCBB0 - 0xCBB7: RST 6, REG */
TEST_RST_REG(B, b, 6, 0xB0);
TEST_RST_REG(C, c, 6, 0xB1);
TEST_RST_REG(D, d, 6, 0xB2);
TEST_RST_REG(E, e, 6, 0xB3);
TEST_RST_REG(H, h, 6, 0xB4);
TEST_RST_REG(L, l, 6, 0xB5);
TEST_RST_MEM_HL(6, 0xB6);
TEST_RST_REG(A, a, 6, 0xB7);

/** 0xCBB8 - 0xCBBF: RST 7, REG */
TEST_RST_REG(B, b, 7, 0xB8);
TEST_RST_REG(C, c, 7, 0xB9);
TEST_RST_REG(D, d, 7, 0xBA);
TEST_RST_REG(E, e, 7, 0xBB);
TEST_RST_REG(H, h, 7, 0xBC);
TEST_RST_REG(L, l, 7, 0xBD);
TEST_RST_MEM_HL(7, 0xBE);
TEST_RST_REG(A, a, 7, 0xBF);

/** 0xCBC0 - 0xCBC7: SET 0, REG */
TEST_SET_REG(B, b, 0, 0xC0);
TEST_SET_REG(C, c, 0, 0xC1);
TEST_SET_REG(D, d, 0, 0xC2);
TEST_SET_REG(E, e, 0, 0xC3);
TEST_SET_REG(H, h, 0, 0xC4);
TEST_SET_REG(L, l, 0, 0xC5);
TEST_SET_MEM_HL(0, 0xC6);
TEST_SET_REG(A, a, 0, 0xC7);

/** 0xCBC8 - 0xCBCF: SET 1, REG */
TEST_SET_REG(B, b, 1, 0xC8);
TEST_SET_REG(C, c, 1, 0xC9);
TEST_SET_REG(D, d, 1, 0xCA);
TEST_SET_REG(E, e, 1, 0xCB);
TEST_SET_REG(H, h, 1, 0xCC);
TEST_SET_REG(L, l, 1, 0xCD);
TEST_SET_MEM_HL(1, 0xCE);
TEST_SET_REG(A, a, 1, 0xCF);

/** 0xCBD0 - 0xCBD7: SET 2, REG */
TEST_SET_REG(B, b, 2, 0xD0);
TEST_SET_REG(C, c, 2, 0xD1);
TEST_SET_REG(D, d, 2, 0xD2);
TEST_SET_REG(E, e, 2, 0xD3);
TEST_SET_REG(H, h, 2, 0xD4);
TEST_SET_REG(L, l, 2, 0xD5);
TEST_SET_MEM_HL(2, 0xD6);
TEST_SET_REG(A, a, 2, 0xD7);

/** 0xCBD8 - 0xCBDF: SET 3, REG */
TEST_SET_REG(B, b, 3, 0xD8);
TEST_SET_REG(C, c, 3, 0xD9);
TEST_SET_REG(D, d, 3, 0xDA);
TEST_SET_REG(E, e, 3, 0xDB);
TEST_SET_REG(H, h, 3, 0xDC);
TEST_SET_REG(L, l, 3, 0xDD);
TEST_SET_MEM_HL(3, 0xDE);
TEST_SET_REG(A, a, 3, 0xDF);

/** 0xCBE0 - 0xCBE7: SET 4, REG */
TEST_SET_REG(B, b, 4, 0xE0);
TEST_SET_REG(C, c, 4, 0xE1);
TEST_SET_REG(D, d, 4, 0xE2);
TEST_SET_REG(E, e, 4, 0xE3);
TEST_SET_REG(H, h, 4, 0xE4);
TEST_SET_REG(L, l, 4, 0xE5);
TEST_SET_MEM_HL(4, 0xE6);
TEST_SET_REG(A, a, 4, 0xE7);

/** 0xCBE8 - 0xCBEF: SET 5, REG */
TEST_SET_REG(B, b, 5, 0xE8);
TEST_SET_REG(C, c, 5, 0xE9);
TEST_SET_REG(D, d, 5, 0xEA);
TEST_SET_REG(E, e, 5, 0xEB);
TEST_SET_REG(H, h, 5, 0xEC);
TEST_SET_REG(L, l, 5, 0xED);
TEST_SET_MEM_HL(5, 0xEE);
TEST_SET_REG(A, a, 5, 0xEF);

/** 0xCBF0 - 0xCBF7: SET 6, REG */
TEST_SET_REG(B, b, 6, 0xF0);
TEST_SET_REG(C, c, 6, 0xF1);
TEST_SET_REG(D, d, 6, 0xF2);
TEST_SET_REG(E, e, 6, 0xF3);
TEST_SET_REG(H, h, 6, 0xF4);
TEST_SET_REG(L, l, 6, 0xF5);
TEST_SET_MEM_HL(6, 0xF6);
TEST_SET_REG(A, a, 6, 0xF7);

/** 0xCBF8 - 0xCBFF: SET 7, REG */
TEST_SET_REG(B, b, 7, 0xF8);
TEST_SET_REG(C, c, 7, 0xF9);
TEST_SET_REG(D, d, 7, 0xFA);
TEST_SET_REG(E, e, 7, 0xFB);
TEST_SET_REG(H, h, 7, 0xFC);
TEST_SET_REG(L, l, 7, 0xFD);
TEST_SET_MEM_HL(7, 0xFE);
TEST_SET_REG(A, a, 7, 0xFF);