#include "cpu.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "status_code.h"
#include "logging.h"

#define INST(handler_fn, dest_operand, src_operand, inst_length, cycle, alt_cycle) \
  ((instruction_t){                                                                \
      .handler = handler_fn,                                                       \
      .operands = {                                                                \
          .op_1 = (uint16_t)dest_operand,                                          \
          .op_2 = (uint16_t)src_operand,                                           \
      },                                                                           \
      .length = inst_length,                                                       \
      .cycle_duration = cycle,                                                     \
      .alt_cycle_duration = alt_cycle,                                             \
  })

/* Addressing mode enum */
typedef enum
{
  AM_NONE,
  AM_IMM_S_8,
  AM_IMM_D_8,
  AM_IMM_D_16,
  AM_IMM_A_16,
  AM_REG_AF,
  AM_REG_BC,
  AM_REG_DE,
  AM_REG_HL,
  AM_REG_SP,
  AM_REG_A,
  AM_REG_B,
  AM_REG_C,
  AM_REG_D,
  AM_REG_E,
  AM_REG_F,
  AM_REG_H,
  AM_REG_L,
  AM_MEM_AF,
  AM_MEM_BC,
  AM_MEM_DE,
  AM_MEM_HL,
  AM_MEM_HL_INC,
  AM_MEM_HL_DEC,
  AM_IMM_FF_A_8,
  AM_MEM_FF_REG_C,
  AM_REG_SP_IMM_S8,
} addressing_mode_t;

typedef enum
{
  JM_UNCOND,
  JM_COND_Z,
  JM_COND_C,
  JM_COND_NZ,
  JM_COND_NC,
} jump_mode_t;

typedef enum
{
  RST_VEC_0 = 0x00,
  RST_VEC_1 = 0x08,
  RST_VEC_2 = 0x10,
  RST_VEC_3 = 0x18,
  RST_VEC_4 = 0x20,
  RST_VEC_5 = 0x28,
  RST_VEC_6 = 0x30,
  RST_VEC_7 = 0x38,
} reset_vector_t;

typedef struct
{
  union
  {
    uint16_t op_1;
    addressing_mode_t dest;
    jump_mode_t jump_mode;
    reset_vector_t reset_vector;
  };
  union
  {
    uint16_t op_2;
    addressing_mode_t source;
  };
} inst_operands_t;

typedef enum
{
  F_CLEAR = 0,
  F_SET,
  F_INVERT,
} flag_mode_t;

typedef union
{
  struct
  {
    uint8_t lsb;
    uint8_t msb;
  };
  uint16_t val;
} address_t;

typedef union
{
  uint8_t u_data;
  int8_t s_data;
} data8_t;

typedef uint8_t *reg_8_ptr_t;
typedef uint16_t *reg_16_ptr_t;
typedef status_code_t (*opcode_handler_fn)(cpu_state_t *const state, inst_operands_t *const operands);

typedef struct instruction_s
{
  opcode_handler_fn handler;
  inst_operands_t operands;
  uint8_t length;
  uint8_t cycle_duration;
  uint8_t alt_cycle_duration;
} instruction_t;

typedef union
{
  status_code_t (*rot_shf_swap_handler_fn)(cpu_state_t *const state, addressing_mode_t const addr_mode);
  status_code_t (*test_set_rst_handler_fn)(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t bit_index);
} cb_opcode_handler_t;

reg_8_ptr_t reg_8_select(cpu_state_t *const state, addressing_mode_t const addr_mode);
reg_16_ptr_t reg_16_select(cpu_state_t *const state, addressing_mode_t const addr_mode);
uint8_t calc_cycle(uint8_t opcode, cpu_state_t *const state);
void update_flags(cpu_state_t *const state, uint8_t mask, flag_mode_t mode);
uint8_t check_cond(cpu_state_t *const state, jump_mode_t mode);

status_code_t read_8(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t *const data);
status_code_t write_8(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const data);
status_code_t read_16(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t *const data);
status_code_t write_16(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t const data);
status_code_t push_reg_16(cpu_state_t *const state, reg_16_ptr_t reg);
status_code_t pop_reg_16(cpu_state_t *const state, reg_16_ptr_t reg);
status_code_t read_reg_16_plus_offset(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t *const data);

static inline status_code_t bus_read_8(cpu_state_t *const state, uint16_t address, uint8_t *const data);
static inline status_code_t bus_write_8(cpu_state_t *const state, uint16_t address, uint8_t const data);
static inline status_code_t bus_read_16(cpu_state_t *const state, uint16_t address, uint16_t *const data);
static inline status_code_t bus_write_16(cpu_state_t *const state, uint16_t address, uint16_t const data);

static status_code_t service_interrupts(cpu_state_t *const state);
static uint8_t handle_single_interrupt(cpu_state_t *const state, interrupt_vector_t *const int_vector);

status_code_t op_NOT_IMPL(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_NOP(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_STOP(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_HALT(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_JR(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_JP(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_LD_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_LD_16(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_INC_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_INC_16(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_DEC_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_DEC_16(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_ADD_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_ADC_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_SUB_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_SBC_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_AND_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_XOR_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_OR_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_CP_8(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_ADD_16(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RLCA(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RRCA(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RLA(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RRA(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_CPL(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_SCF(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_CCF(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RET(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_CALL(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_POP(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_PUSH(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RST(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_DAA(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_RETI(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_DI(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_EI(cpu_state_t *const state, inst_operands_t *const operands);
status_code_t op_PRCB(cpu_state_t *const state, inst_operands_t *const operands);

/* Ops for CB-prefixed opcodes */
status_code_t op_RLC(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_RRC(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_RL(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_RR(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_SLA(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_SRA(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_SWAP(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_SRL(cpu_state_t *const state, addressing_mode_t const addr_mode);
status_code_t op_BIT(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index);
status_code_t op_RES(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index);
status_code_t op_SET(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index);

static instruction_t inst_table[] = {
    /* 0x00 - 0x0F */
    [0x00] = INST(op_NOP, AM_NONE, AM_NONE, 1, 1, 0),
    [0x01] = INST(op_LD_16, AM_REG_BC, AM_IMM_D_16, 3, 3, 0),
    [0x02] = INST(op_LD_8, AM_MEM_BC, AM_REG_A, 1, 2, 0),
    [0x03] = INST(op_INC_16, AM_REG_BC, AM_NONE, 1, 2, 0),

    [0x04] = INST(op_INC_8, AM_REG_B, AM_NONE, 1, 1, 0),
    [0x05] = INST(op_DEC_8, AM_REG_B, AM_NONE, 1, 1, 0),
    [0x06] = INST(op_LD_8, AM_REG_B, AM_IMM_D_8, 2, 2, 0),
    [0x07] = INST(op_RLCA, AM_NONE, AM_NONE, 1, 1, 0),

    [0x08] = INST(op_LD_16, AM_IMM_A_16, AM_REG_SP, 3, 5, 0),
    [0x09] = INST(op_ADD_16, AM_REG_HL, AM_REG_BC, 1, 2, 0),
    [0x0A] = INST(op_LD_8, AM_REG_A, AM_MEM_BC, 1, 2, 0),
    [0x0B] = INST(op_DEC_16, AM_REG_BC, AM_NONE, 1, 2, 0),

    [0x0C] = INST(op_INC_8, AM_REG_C, AM_NONE, 1, 1, 0),
    [0x0D] = INST(op_DEC_8, AM_REG_C, AM_NONE, 1, 1, 0),
    [0x0E] = INST(op_LD_8, AM_REG_C, AM_IMM_D_8, 2, 2, 0),
    [0x0F] = INST(op_RRCA, AM_NONE, AM_NONE, 1, 1, 0),

    /* 0x10 - 0x1F */
    [0x10] = INST(op_STOP, AM_NONE, AM_NONE, 2, 1, 0),
    [0x11] = INST(op_LD_16, AM_REG_DE, AM_IMM_D_16, 3, 3, 0),
    [0x12] = INST(op_LD_8, AM_MEM_DE, AM_REG_A, 1, 2, 0),
    [0x13] = INST(op_INC_16, AM_REG_DE, AM_NONE, 1, 2, 0),

    [0x14] = INST(op_INC_8, AM_REG_D, AM_NONE, 1, 1, 0),
    [0x15] = INST(op_DEC_8, AM_REG_D, AM_NONE, 1, 1, 0),
    [0x16] = INST(op_LD_8, AM_REG_D, AM_IMM_D_8, 2, 2, 0),
    [0x17] = INST(op_RLA, AM_NONE, AM_NONE, 1, 1, 0),

    [0x18] = INST(op_JR, JM_UNCOND, AM_IMM_S_8, 2, 3, 0),
    [0x19] = INST(op_ADD_16, AM_REG_HL, AM_REG_DE, 1, 2, 0),
    [0x1A] = INST(op_LD_8, AM_REG_A, AM_MEM_DE, 1, 2, 0),
    [0x1B] = INST(op_DEC_16, AM_REG_DE, AM_NONE, 1, 2, 0),

    [0x1C] = INST(op_INC_8, AM_REG_E, AM_NONE, 1, 1, 0),
    [0x1D] = INST(op_DEC_8, AM_REG_E, AM_NONE, 1, 1, 0),
    [0x1E] = INST(op_LD_8, AM_REG_E, AM_IMM_D_8, 2, 2, 0),
    [0x1F] = INST(op_RRA, AM_NONE, AM_NONE, 1, 1, 0),

    /* 0x20 - 0x2F */
    [0x20] = INST(op_JR, JM_COND_NZ, AM_IMM_S_8, 2, 3, 2),
    [0x21] = INST(op_LD_16, AM_REG_HL, AM_IMM_D_16, 3, 3, 0),
    [0x22] = INST(op_LD_8, AM_MEM_HL_INC, AM_REG_A, 1, 2, 0),
    [0x23] = INST(op_INC_16, AM_REG_HL, AM_NONE, 1, 2, 0),

    [0x24] = INST(op_INC_8, AM_REG_H, AM_NONE, 1, 1, 0),
    [0x25] = INST(op_DEC_8, AM_REG_H, AM_NONE, 1, 1, 0),
    [0x26] = INST(op_LD_8, AM_REG_H, AM_IMM_D_8, 2, 2, 0),
    [0x27] = INST(op_DAA, AM_NONE, AM_NONE, 1, 1, 0),

    [0x28] = INST(op_JR, JM_COND_Z, AM_IMM_S_8, 2, 3, 2),
    [0x29] = INST(op_ADD_16, AM_REG_HL, AM_REG_HL, 1, 2, 0),
    [0x2A] = INST(op_LD_8, AM_REG_A, AM_MEM_HL_INC, 1, 2, 0),
    [0x2B] = INST(op_DEC_16, AM_REG_HL, AM_NONE, 1, 2, 0),

    [0x2C] = INST(op_INC_8, AM_REG_L, AM_NONE, 1, 1, 0),
    [0x2D] = INST(op_DEC_8, AM_REG_L, AM_NONE, 1, 1, 0),
    [0x2E] = INST(op_LD_8, AM_REG_L, AM_IMM_D_8, 2, 2, 0),
    [0x2F] = INST(op_CPL, AM_NONE, AM_NONE, 1, 1, 0),

    /* 0x30 - 0x3F */
    [0x30] = INST(op_JR, JM_COND_NC, AM_IMM_S_8, 2, 3, 2),
    [0x31] = INST(op_LD_16, AM_REG_SP, AM_IMM_D_16, 3, 3, 0),
    [0x32] = INST(op_LD_8, AM_MEM_HL_DEC, AM_REG_A, 1, 2, 0),
    [0x33] = INST(op_INC_16, AM_REG_SP, AM_NONE, 1, 2, 0),

    [0x34] = INST(op_INC_8, AM_MEM_HL, AM_NONE, 1, 3, 0),
    [0x35] = INST(op_DEC_8, AM_MEM_HL, AM_NONE, 1, 3, 0),
    [0x36] = INST(op_LD_8, AM_MEM_HL, AM_IMM_D_8, 2, 3, 0),
    [0x37] = INST(op_SCF, AM_NONE, AM_NONE, 1, 1, 0),

    [0x38] = INST(op_JR, JM_COND_C, AM_IMM_S_8, 2, 3, 2),
    [0x39] = INST(op_ADD_16, AM_REG_HL, AM_REG_SP, 1, 2, 0),
    [0x3A] = INST(op_LD_8, AM_REG_A, AM_MEM_HL_DEC, 1, 2, 0),
    [0x3B] = INST(op_DEC_16, AM_REG_SP, AM_NONE, 1, 2, 0),

    [0x3C] = INST(op_INC_8, AM_REG_A, AM_NONE, 1, 1, 0),
    [0x3D] = INST(op_DEC_8, AM_REG_A, AM_NONE, 1, 1, 0),
    [0x3E] = INST(op_LD_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0x3F] = INST(op_CCF, AM_NONE, AM_NONE, 1, 1, 0),

    /* 0x40 - 0x4F */
    [0x40] = INST(op_LD_8, AM_REG_B, AM_REG_B, 1, 1, 0),
    [0x41] = INST(op_LD_8, AM_REG_B, AM_REG_C, 1, 1, 0),
    [0x42] = INST(op_LD_8, AM_REG_B, AM_REG_D, 1, 1, 0),
    [0x43] = INST(op_LD_8, AM_REG_B, AM_REG_E, 1, 1, 0),

    [0x44] = INST(op_LD_8, AM_REG_B, AM_REG_H, 1, 1, 0),
    [0x45] = INST(op_LD_8, AM_REG_B, AM_REG_L, 1, 1, 0),
    [0x46] = INST(op_LD_8, AM_REG_B, AM_MEM_HL, 1, 2, 0),
    [0x47] = INST(op_LD_8, AM_REG_B, AM_REG_A, 1, 1, 0),

    [0x48] = INST(op_LD_8, AM_REG_C, AM_REG_B, 1, 1, 0),
    [0x49] = INST(op_LD_8, AM_REG_C, AM_REG_C, 1, 1, 0),
    [0x4A] = INST(op_LD_8, AM_REG_C, AM_REG_D, 1, 1, 0),
    [0x4B] = INST(op_LD_8, AM_REG_C, AM_REG_E, 1, 1, 0),

    [0x4C] = INST(op_LD_8, AM_REG_C, AM_REG_H, 1, 1, 0),
    [0x4D] = INST(op_LD_8, AM_REG_C, AM_REG_L, 1, 1, 0),
    [0x4E] = INST(op_LD_8, AM_REG_C, AM_MEM_HL, 1, 2, 0),
    [0x4F] = INST(op_LD_8, AM_REG_C, AM_REG_A, 1, 1, 0),

    /* 0x50 - 0x5F */
    [0x50] = INST(op_LD_8, AM_REG_D, AM_REG_B, 1, 1, 0),
    [0x51] = INST(op_LD_8, AM_REG_D, AM_REG_C, 1, 1, 0),
    [0x52] = INST(op_LD_8, AM_REG_D, AM_REG_D, 1, 1, 0),
    [0x53] = INST(op_LD_8, AM_REG_D, AM_REG_E, 1, 1, 0),

    [0x54] = INST(op_LD_8, AM_REG_D, AM_REG_H, 1, 1, 0),
    [0x55] = INST(op_LD_8, AM_REG_D, AM_REG_L, 1, 1, 0),
    [0x56] = INST(op_LD_8, AM_REG_D, AM_MEM_HL, 1, 2, 0),
    [0x57] = INST(op_LD_8, AM_REG_D, AM_REG_A, 1, 1, 0),

    [0x58] = INST(op_LD_8, AM_REG_E, AM_REG_B, 1, 1, 0),
    [0x59] = INST(op_LD_8, AM_REG_E, AM_REG_C, 1, 1, 0),
    [0x5A] = INST(op_LD_8, AM_REG_E, AM_REG_D, 1, 1, 0),
    [0x5B] = INST(op_LD_8, AM_REG_E, AM_REG_E, 1, 1, 0),

    [0x5C] = INST(op_LD_8, AM_REG_E, AM_REG_H, 1, 1, 0),
    [0x5D] = INST(op_LD_8, AM_REG_E, AM_REG_L, 1, 1, 0),
    [0x5E] = INST(op_LD_8, AM_REG_E, AM_MEM_HL, 1, 2, 0),
    [0x5F] = INST(op_LD_8, AM_REG_E, AM_REG_A, 1, 1, 0),

    /* 0x60 - 0x6F */
    [0x60] = INST(op_LD_8, AM_REG_H, AM_REG_B, 1, 1, 0),
    [0x61] = INST(op_LD_8, AM_REG_H, AM_REG_C, 1, 1, 0),
    [0x62] = INST(op_LD_8, AM_REG_H, AM_REG_D, 1, 1, 0),
    [0x63] = INST(op_LD_8, AM_REG_H, AM_REG_E, 1, 1, 0),

    [0x64] = INST(op_LD_8, AM_REG_H, AM_REG_H, 1, 1, 0),
    [0x65] = INST(op_LD_8, AM_REG_H, AM_REG_L, 1, 1, 0),
    [0x66] = INST(op_LD_8, AM_REG_H, AM_MEM_HL, 1, 2, 0),
    [0x67] = INST(op_LD_8, AM_REG_H, AM_REG_A, 1, 1, 0),

    [0x68] = INST(op_LD_8, AM_REG_L, AM_REG_B, 1, 1, 0),
    [0x69] = INST(op_LD_8, AM_REG_L, AM_REG_C, 1, 1, 0),
    [0x6A] = INST(op_LD_8, AM_REG_L, AM_REG_D, 1, 1, 0),
    [0x6B] = INST(op_LD_8, AM_REG_L, AM_REG_E, 1, 1, 0),

    [0x6C] = INST(op_LD_8, AM_REG_L, AM_REG_H, 1, 1, 0),
    [0x6D] = INST(op_LD_8, AM_REG_L, AM_REG_L, 1, 1, 0),
    [0x6E] = INST(op_LD_8, AM_REG_L, AM_MEM_HL, 1, 2, 0),
    [0x6F] = INST(op_LD_8, AM_REG_L, AM_REG_A, 1, 1, 0),

    /* 0x70 - 0x7F */
    [0x70] = INST(op_LD_8, AM_MEM_HL, AM_REG_B, 1, 2, 0),
    [0x71] = INST(op_LD_8, AM_MEM_HL, AM_REG_C, 1, 2, 0),
    [0x72] = INST(op_LD_8, AM_MEM_HL, AM_REG_D, 1, 2, 0),
    [0x73] = INST(op_LD_8, AM_MEM_HL, AM_REG_E, 1, 2, 0),

    [0x74] = INST(op_LD_8, AM_MEM_HL, AM_REG_H, 1, 2, 0),
    [0x75] = INST(op_LD_8, AM_MEM_HL, AM_REG_L, 1, 2, 0),
    [0x76] = INST(op_HALT, AM_NONE, AM_NONE, 1, 1, 0),
    [0x77] = INST(op_LD_8, AM_MEM_HL, AM_REG_A, 1, 2, 0),

    [0x78] = INST(op_LD_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0x79] = INST(op_LD_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0x7A] = INST(op_LD_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0x7B] = INST(op_LD_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0x7C] = INST(op_LD_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0x7D] = INST(op_LD_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0x7E] = INST(op_LD_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0x7F] = INST(op_LD_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    /* 0x80 - 0x8F */
    [0x80] = INST(op_ADD_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0x81] = INST(op_ADD_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0x82] = INST(op_ADD_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0x83] = INST(op_ADD_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0x84] = INST(op_ADD_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0x85] = INST(op_ADD_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0x86] = INST(op_ADD_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0x87] = INST(op_ADD_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    [0x88] = INST(op_ADC_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0x89] = INST(op_ADC_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0x8A] = INST(op_ADC_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0x8B] = INST(op_ADC_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0x8C] = INST(op_ADC_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0x8D] = INST(op_ADC_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0x8E] = INST(op_ADC_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0x8F] = INST(op_ADC_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    /* 0x90 - 0x9F */
    [0x90] = INST(op_SUB_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0x91] = INST(op_SUB_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0x92] = INST(op_SUB_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0x93] = INST(op_SUB_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0x94] = INST(op_SUB_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0x95] = INST(op_SUB_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0x96] = INST(op_SUB_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0x97] = INST(op_SUB_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    [0x98] = INST(op_SBC_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0x99] = INST(op_SBC_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0x9A] = INST(op_SBC_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0x9B] = INST(op_SBC_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0x9C] = INST(op_SBC_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0x9D] = INST(op_SBC_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0x9E] = INST(op_SBC_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0x9F] = INST(op_SBC_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    /* 0xA0 - 0xAF */
    [0xA0] = INST(op_AND_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0xA1] = INST(op_AND_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0xA2] = INST(op_AND_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0xA3] = INST(op_AND_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0xA4] = INST(op_AND_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0xA5] = INST(op_AND_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0xA6] = INST(op_AND_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0xA7] = INST(op_AND_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    [0xA8] = INST(op_XOR_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0xA9] = INST(op_XOR_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0xAA] = INST(op_XOR_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0xAB] = INST(op_XOR_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0xAC] = INST(op_XOR_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0xAD] = INST(op_XOR_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0xAE] = INST(op_XOR_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0xAF] = INST(op_XOR_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    /* 0xB0 - 0xBF */
    [0xB0] = INST(op_OR_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0xB1] = INST(op_OR_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0xB2] = INST(op_OR_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0xB3] = INST(op_OR_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0xB4] = INST(op_OR_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0xB5] = INST(op_OR_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0xB6] = INST(op_OR_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0xB7] = INST(op_OR_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    [0xB8] = INST(op_CP_8, AM_REG_A, AM_REG_B, 1, 1, 0),
    [0xB9] = INST(op_CP_8, AM_REG_A, AM_REG_C, 1, 1, 0),
    [0xBA] = INST(op_CP_8, AM_REG_A, AM_REG_D, 1, 1, 0),
    [0xBB] = INST(op_CP_8, AM_REG_A, AM_REG_E, 1, 1, 0),

    [0xBC] = INST(op_CP_8, AM_REG_A, AM_REG_H, 1, 1, 0),
    [0xBD] = INST(op_CP_8, AM_REG_A, AM_REG_L, 1, 1, 0),
    [0xBE] = INST(op_CP_8, AM_REG_A, AM_MEM_HL, 1, 2, 0),
    [0xBF] = INST(op_CP_8, AM_REG_A, AM_REG_A, 1, 1, 0),

    /* 0xC0 - 0xCF */
    [0xC0] = INST(op_RET, JM_COND_NZ, AM_NONE, 1, 5, 2),
    [0xC1] = INST(op_POP, AM_REG_BC, AM_NONE, 1, 3, 0),
    [0xC2] = INST(op_JP, JM_COND_NZ, AM_IMM_A_16, 3, 4, 3),
    [0xC3] = INST(op_JP, JM_UNCOND, AM_IMM_A_16, 3, 4, 0),

    [0xC4] = INST(op_CALL, JM_COND_NZ, AM_IMM_A_16, 3, 6, 3),
    [0xC5] = INST(op_PUSH, AM_REG_BC, AM_NONE, 1, 4, 0),
    [0xC6] = INST(op_ADD_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xC7] = INST(op_RST, RST_VEC_0, AM_NONE, 1, 4, 0),

    [0xC8] = INST(op_RET, JM_COND_Z, AM_NONE, 1, 5, 2),
    [0xC9] = INST(op_RET, JM_UNCOND, AM_NONE, 1, 4, 0),
    [0xCA] = INST(op_JP, JM_COND_Z, AM_IMM_A_16, 3, 4, 3),
    [0xCB] = INST(op_PRCB, AM_NONE, AM_NONE, 1, 0, 0),

    [0xCC] = INST(op_CALL, JM_COND_Z, AM_IMM_A_16, 3, 6, 3),
    [0xCD] = INST(op_CALL, JM_UNCOND, AM_IMM_A_16, 3, 6, 0),
    [0xCE] = INST(op_ADC_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xCF] = INST(op_RST, RST_VEC_1, AM_NONE, 1, 4, 0),

    /* 0xD0 - 0xDF */
    [0xD0] = INST(op_RET, JM_COND_NC, AM_NONE, 1, 5, 2),
    [0xD1] = INST(op_POP, AM_REG_DE, AM_NONE, 1, 3, 0),
    [0xD2] = INST(op_JP, JM_COND_NC, AM_IMM_A_16, 3, 4, 3),
    [0xD3] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),

    [0xD4] = INST(op_CALL, JM_COND_NC, AM_IMM_A_16, 3, 6, 3),
    [0xD5] = INST(op_PUSH, AM_REG_DE, AM_NONE, 1, 4, 0),
    [0xD6] = INST(op_SUB_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xD7] = INST(op_RST, RST_VEC_2, AM_NONE, 1, 4, 0),

    [0xD8] = INST(op_RET, JM_COND_C, AM_NONE, 1, 5, 2),
    [0xD9] = INST(op_RETI, AM_NONE, AM_NONE, 1, 4, 0),
    [0xDA] = INST(op_JP, JM_COND_C, AM_IMM_A_16, 3, 4, 3),
    [0xDB] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),

    [0xDC] = INST(op_CALL, JM_COND_C, AM_IMM_A_16, 3, 6, 3),
    [0xDD] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xDE] = INST(op_SBC_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xDF] = INST(op_RST, RST_VEC_3, AM_NONE, 1, 4, 0),

    /* 0xE0 - 0xEF */
    [0xE0] = INST(op_LD_8, AM_IMM_FF_A_8, AM_REG_A, 2, 3, 0),
    [0xE1] = INST(op_POP, AM_REG_HL, AM_NONE, 1, 3, 0),
    [0xE2] = INST(op_LD_8, AM_MEM_FF_REG_C, AM_REG_A, 1, 2, 0),
    [0xE3] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),

    [0xE4] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xE5] = INST(op_PUSH, AM_REG_HL, AM_NONE, 1, 4, 0),
    [0xE6] = INST(op_AND_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xE7] = INST(op_RST, RST_VEC_4, AM_NONE, 1, 4, 0),

    [0xE8] = INST(op_ADD_16, AM_REG_SP, AM_IMM_S_8, 2, 4, 0),
    [0xE9] = INST(op_JP, JM_UNCOND, AM_REG_HL, 1, 1, 0),
    [0xEA] = INST(op_LD_8, AM_IMM_A_16, AM_REG_A, 3, 4, 0),
    [0xEB] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),

    [0xEC] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xED] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xEE] = INST(op_XOR_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xEF] = INST(op_RST, RST_VEC_5, AM_NONE, 1, 4, 0),

    /* 0xF0 - 0xFF */
    [0xF0] = INST(op_LD_8, AM_REG_A, AM_IMM_FF_A_8, 2, 3, 0),
    [0xF1] = INST(op_POP, AM_REG_AF, AM_NONE, 1, 3, 0),
    [0xF2] = INST(op_LD_8, AM_REG_A, AM_MEM_FF_REG_C, 1, 2, 0),
    [0xF3] = INST(op_DI, AM_NONE, AM_NONE, 1, 1, 0),

    [0xF4] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xF5] = INST(op_PUSH, AM_REG_AF, AM_NONE, 1, 4, 0),
    [0xF6] = INST(op_OR_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xF7] = INST(op_RST, RST_VEC_6, AM_NONE, 1, 4, 0),

    [0xF8] = INST(op_LD_16, AM_REG_HL, AM_REG_SP_IMM_S8, 2, 3, 0),
    [0xF9] = INST(op_LD_16, AM_REG_SP, AM_REG_HL, 1, 2, 0),
    [0xFA] = INST(op_LD_8, AM_REG_A, AM_IMM_A_16, 3, 4, 0),
    [0xFB] = INST(op_EI, AM_NONE, AM_NONE, 1, 1, 0),

    [0xFC] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xFD] = INST(op_NOT_IMPL, AM_NONE, AM_NONE, 1, 0, 0),
    [0xFE] = INST(op_CP_8, AM_REG_A, AM_IMM_D_8, 2, 2, 0),
    [0xFF] = INST(op_RST, RST_VEC_7, AM_NONE, 1, 4, 0),
};

static interrupt_vector_t interrupt_vector_table[] = {
    {.address = 0x0040, .int_type = INT_VBLANK},
    {.address = 0x0048, .int_type = INT_LCD},
    {.address = 0x0050, .int_type = INT_TIMER},
    {.address = 0x0058, .int_type = INT_SERIAL},
    {.address = 0x0060, .int_type = INT_JOYPAD},
};

status_code_t cpu_init(cpu_state_t *const state, cpu_init_param_t *const param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(state);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(param);

  memset(state, 0, sizeof(cpu_state_t));
  state->registers.pc = ENTRY_PT_ADDR;
  state->run_mode = RUN_MODE_NORMAL;
  state->registers.f = 0x00;
  state->ime_flag = 0;
  state->bus_interface.read = param->bus_read_fn;
  state->bus_interface.write = param->bus_write_fn;
  state->bus_interface.resource = param->bus_resource;

  return STATUS_OK;
}

status_code_t cpu_emulation_cycle(cpu_state_t *const state)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(state);

  status_code_t status = STATUS_OK;
  registers_t *const regs = &state->registers;
  uint8_t service_interrupt = state->ime_flag;

  if (state->next_ime_flag)
  {
    state->ime_flag = 1;
  }

  if (state->run_mode == RUN_MODE_HALTED)
  {
    if (state->int_state.int_requested_flag)
    {
      state->run_mode = RUN_MODE_NORMAL;
    }
  }
  else if (service_interrupt)
  {
    status = service_interrupts(state);
  }
  else if (state->run_mode == RUN_MODE_NORMAL)
  {
    uint8_t opcode;
    uint16_t pc = regs->pc;

    status = bus_read_8(state, regs->pc++, &opcode);
    RETURN_STATUS_IF_NOT_OK(status);

    instruction_t *const inst = &inst_table[opcode];

    Log_D("Exec [%04X] -- %02X", pc, opcode);

    status = inst->handler(state, &inst->operands);
    RETURN_STATUS_IF_NOT_OK(status);

    state->m_cycles += calc_cycle(opcode, state);
  }

  return STATUS_OK;
}

static uint8_t handle_single_interrupt(cpu_state_t *const state, interrupt_vector_t *const int_vector)
{
  interrupt_state_t *const interrupt = &state->int_state;
  if (interrupt->int_enable_mask & interrupt->int_requested_flag & int_vector->int_type)
  {
    /**
     * Save current instruction to the stack and go to
     * the address specified by the interrupt vector
     */
    push_reg_16(state, &state->registers.pc);
    state->registers.pc = int_vector->address;

    /* Clear the request flag for this inetrrupt */
    interrupt->int_requested_flag &= ~int_vector->int_type;

    /* Exit HALT mode if tthe CPU is currently halted */
    if (state->run_mode == RUN_MODE_HALTED)
    {
      state->run_mode = RUN_MODE_NORMAL;
    }

    /* Disable interrupt master enable */
    state->ime_flag = 0;

    return 1;
  }

  return 0;
}

static status_code_t service_interrupts(cpu_state_t *const state)
{
  for (int8_t i = 0; i < 5; i++)
  {
    if (handle_single_interrupt(state, &interrupt_vector_table[i]))
    {
      return STATUS_OK;
    }
  }
  return STATUS_OK;
}

reg_8_ptr_t reg_8_select(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  registers_t *const regs = &state->registers;

  switch (addr_mode)
  {
  case AM_REG_A:
    return &regs->a;
  case AM_REG_B:
    return &regs->b;
  case AM_REG_C:
    return &regs->c;
  case AM_REG_D:
    return &regs->d;
  case AM_REG_E:
    return &regs->e;
  case AM_REG_H:
    return &regs->h;
  case AM_REG_L:
    return &regs->l;
  default:
    break;
  }

  return NULL;
}

reg_16_ptr_t reg_16_select(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  registers_t *const regs = &state->registers;

  switch (addr_mode)
  {
  case AM_REG_AF:
    return &regs->af;
  case AM_REG_BC:
    return &regs->bc;
  case AM_REG_DE:
    return &regs->de;
  case AM_REG_HL:
    return &regs->hl;
  case AM_REG_SP:
  case AM_REG_SP_IMM_S8:
    return &regs->sp;
  default:
    break;
  }

  return NULL;
}

static inline status_code_t bus_read_8(cpu_state_t *const state, uint16_t address, uint8_t *const data)
{
  return state->bus_interface.read(state->bus_interface.resource, address, data);
}

static inline status_code_t bus_write_8(cpu_state_t *const state, uint16_t address, uint8_t const data)
{
  return state->bus_interface.write(state->bus_interface.resource, address, data);
}

static inline status_code_t bus_read_16(cpu_state_t *const state, uint16_t address, uint16_t *const data)
{
  uint8_t lsb, msb;
  status_code_t status;
  status = state->bus_interface.read(state->bus_interface.resource, address, &lsb);
  status = state->bus_interface.read(state->bus_interface.resource, address + 1, &msb);
  *data = (msb << 8) | lsb;
  return status;
}

static inline status_code_t bus_write_16(cpu_state_t *const state, uint16_t address, uint16_t const data)
{
  status_code_t status;
  status = state->bus_interface.write(state->bus_interface.resource, address, (data & 0xFF));
  status = state->bus_interface.write(state->bus_interface.resource, address + 1, (data >> 8) & 0xFF);
  return status;
}

status_code_t read_8(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t *const data)
{
  address_t address;
  reg_8_ptr_t source;
  registers_t *const regs = &state->registers;
  status_code_t status = STATUS_OK;

  switch (addr_mode)
  {
  case AM_IMM_D_8:
  case AM_IMM_S_8:
    return bus_read_8(state, regs->pc++, data);
  case AM_MEM_HL:
  case AM_MEM_HL_INC:
  case AM_MEM_HL_DEC:
    return bus_read_8(state, regs->hl, data);
  case AM_MEM_BC:
    return bus_read_8(state, regs->bc, data);
  case AM_MEM_DE:
    return bus_read_8(state, regs->de, data);
  case AM_MEM_FF_REG_C:
    return bus_read_8(state, (0xFF00 | regs->c), data);
  case AM_IMM_FF_A_8:
    status = bus_read_8(state, regs->pc++, &address.lsb); /* TODO check status */
    status = bus_read_8(state, (0xFF00 | address.lsb), data);
    return status;
  case AM_IMM_A_16:
    status = read_16(state, addr_mode, &address.val); /* TODO check status */
    status = bus_read_8(state, address.val, data);
    return status;
  default:
    if ((source = reg_8_select(state, addr_mode)) == NULL)
    {
      return STATUS_ERR_INVALID_ARG;
    }
    *data = *source;
    break;
  }

  return STATUS_OK;
}

status_code_t write_8(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const data)
{
  address_t address;
  reg_8_ptr_t source;
  registers_t *const regs = &state->registers;
  status_code_t status = STATUS_OK;

  switch (addr_mode)
  {
  case AM_MEM_HL:
  case AM_MEM_HL_INC:
  case AM_MEM_HL_DEC:
    return bus_write_8(state, regs->hl, data);
  case AM_MEM_BC:
    return bus_write_8(state, regs->bc, data);
  case AM_MEM_DE:
    return bus_write_8(state, regs->de, data);
  case AM_MEM_FF_REG_C:
    return bus_write_8(state, (0xFF00 | regs->c), data);
  case AM_IMM_FF_A_8:
    status = bus_read_8(state, regs->pc++, &address.lsb); /* TODO check status */
    status = bus_write_8(state, (0xFF00 | address.lsb), data);
    return status;
  case AM_IMM_A_16:
    status = read_16(state, addr_mode, &address.val); /* TODO check status */
    status = bus_write_8(state, address.val, data);
    return status;
  default:
    if ((source = reg_8_select(state, addr_mode)) == NULL)
    {
      return STATUS_ERR_INVALID_ARG;
    }
    *source = data;
    break;
  }

  return STATUS_OK;
}

status_code_t read_16(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t *const data)
{
  reg_16_ptr_t source;
  registers_t *const regs = &state->registers;
  status_code_t status = STATUS_OK;

  switch (addr_mode)
  {
  case AM_IMM_A_16:
  case AM_IMM_D_16:
    status = bus_read_16(state, regs->pc, data);
    regs->pc += 2;
    return status;
  default:
    if ((source = reg_16_select(state, addr_mode)) == NULL)
    {
      return STATUS_ERR_INVALID_ARG;
    }
    *data = *source;
    break;
  }

  return STATUS_OK;
}

status_code_t write_16(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t const data)
{
  uint16_t address;
  status_code_t status = STATUS_OK;

  if (addr_mode == AM_IMM_A_16)
  {
    status = bus_read_16(state, state->registers.pc, &address);
    status = bus_write_16(state, address, data);
    state->registers.pc += 2;
  }
  else
  {
    reg_16_ptr_t dest = reg_16_select(state, addr_mode);
    if (dest == NULL)
    {
      return STATUS_ERR_INVALID_ARG;
    }
    *dest = data;
  }

  return status;
}

status_code_t pop_reg_16(cpu_state_t *const state, reg_16_ptr_t reg)
{
  status_code_t status = STATUS_OK;
  status = bus_read_16(state, state->registers.sp, reg);
  state->registers.sp += 2;

  return status;
}

status_code_t push_reg_16(cpu_state_t *const state, reg_16_ptr_t reg)
{
  state->registers.sp -= 2;
  return bus_write_16(state, state->registers.sp, *reg);
}

void update_flags(cpu_state_t *const state, uint8_t mask, flag_mode_t mode)
{
  registers_t *const regs = &state->registers;

  switch (mode)
  {
  case F_SET:
    regs->f |= mask;
    break;
  case F_CLEAR:
    regs->f &= ~mask;
    break;
  case F_INVERT:
    regs->f ^= mask;
    break;
  default:
    break;
  }
}

uint8_t check_cond(cpu_state_t *const state, jump_mode_t mode)
{
  registers_t *const regs = &state->registers;

  switch (mode)
  {
  case JM_UNCOND:
    return 1;
  case JM_COND_NZ:
    return (regs->f & FLAG_Z) ? 0 : 1;
  case JM_COND_Z:
    return (regs->f & FLAG_Z) ? 1 : 0;
  case JM_COND_NC:
    return (regs->f & FLAG_C) ? 0 : 1;
  case JM_COND_C:
    return (regs->f & FLAG_C) ? 1 : 0;
  default:
    break;
  }
  return 0;
}

uint8_t calc_cycle(uint8_t opcode, cpu_state_t *const state)
{
  registers_t *const regs = &state->registers;
  instruction_t *const inst = &inst_table[opcode];
  switch (opcode)
  {
  case 0x20: /* JR NZ, s8 */
  case 0xC0: /* RET NZ */
  case 0xC2: /* JP NZ, a16 */
  case 0xC4: /* CALL NZ, a16 */
    return (regs->f & FLAG_Z) ? inst->alt_cycle_duration : inst->cycle_duration;
  case 0x28: /* JR Z, s8 */
  case 0xC8: /* RET Z */
  case 0xCA: /* JP Z, a16 */
  case 0xCC: /* CALL Z, a16 */
    return (regs->f & FLAG_Z) ? inst->cycle_duration : inst->alt_cycle_duration;
  case 0x30: /* JR NC, s8 */
  case 0xD0: /* RET NC */
  case 0xD2: /* JP NC, a16 */
  case 0xD4: /* CALL NC, a16 */
    return (regs->f & FLAG_C) ? inst->alt_cycle_duration : inst->cycle_duration;
  case 0x38: /* JR C, s8 */
  case 0xD8: /* RET C */
  case 0xDA: /* JP C, a16 */
  case 0xDC: /* CALL C, a16 */
    return (regs->f & FLAG_C) ? inst->cycle_duration : inst->alt_cycle_duration;
  default:
    break;
  }

  return inst->cycle_duration;
}

status_code_t read_reg_16_plus_offset(cpu_state_t *const state, addressing_mode_t const addr_mode, uint16_t *const data)
{
  int8_t offset;
  status_code_t status = STATUS_OK;

  status = read_8(state, AM_IMM_S_8, (uint8_t *)&offset);
  RETURN_STATUS_IF_NOT_OK(status);

  status = read_16(state, addr_mode, data);
  RETURN_STATUS_IF_NOT_OK(status);

  uint8_t half_carry = ((*data & 0x0F) + (offset & 0x0F)) & 0x10;
  uint16_t full_carry = ((*data & 0xFF) + (offset & 0xFF)) & 0x100;

  *data = *data + offset;

  update_flags(state, FLAG_Z | FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_NOT_IMPL(cpu_state_t *const __attribute__((unused)) state, inst_operands_t *const __attribute__((unused)) operands)
{
  return STATUS_ERR_UNDEFINED_INST;
}

status_code_t op_NOP(cpu_state_t *const __attribute__((unused)) state, inst_operands_t *const __attribute__((unused)) operands)
{
  return STATUS_OK;
}

status_code_t op_STOP(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  state->registers.pc++;
  state->run_mode = RUN_MODE_STOPPED;
  return STATUS_OK;
}

status_code_t op_HALT(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  state->run_mode = RUN_MODE_HALTED;
  return STATUS_OK;
}

status_code_t op_JR(cpu_state_t *const state, inst_operands_t *const operands)
{
  int8_t offset;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, (uint8_t *)&offset);
  RETURN_STATUS_IF_NOT_OK(status);

  if (check_cond(state, operands->jump_mode))
  {
    state->registers.pc += offset;
  }
  return status;
}

status_code_t op_JP(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint16_t address;
  status_code_t status = STATUS_OK;

  status = read_16(state, operands->source, &address);
  RETURN_STATUS_IF_NOT_OK(status);

  if (check_cond(state, operands->jump_mode))
  {
    state->registers.pc = address;
  }

  return status;
}

status_code_t op_LD_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data = 0;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  status = write_8(state, operands->dest, data);

  if ((operands->dest == AM_MEM_HL_INC) || (operands->source == AM_MEM_HL_INC))
  {
    state->registers.hl++;
  }
  else if ((operands->dest == AM_MEM_HL_DEC) || (operands->source == AM_MEM_HL_DEC))
  {
    state->registers.hl--;
  }

  return status;
}

status_code_t op_LD_16(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint16_t data = 0;
  status_code_t status = STATUS_OK;

  if (operands->source == AM_REG_SP_IMM_S8)
  {
    status = read_reg_16_plus_offset(state, operands->source, &data);
  }
  else
  {
    status = read_16(state, operands->source, &data);
  }
  RETURN_STATUS_IF_NOT_OK(status);

  status = write_16(state, operands->dest, data);
  return status;
}

status_code_t op_INC_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->dest, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  status = write_8(state, operands->dest, ++data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, ((data & 0xF) == 0x0) ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_INC_16(cpu_state_t *const state, inst_operands_t *const operands)
{
  status_code_t status = STATUS_OK;
  reg_16_ptr_t dest = reg_16_select(state, operands->dest);
  if (dest != NULL)
  {
    ++(*dest);
  }
  return status;
}

status_code_t op_DEC_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->dest, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  status = write_8(state, operands->dest, --data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_SET);
  update_flags(state, FLAG_H, ((data & 0xF) == 0xF) ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_DEC_16(cpu_state_t *const state, inst_operands_t *const operands)
{
  status_code_t status = STATUS_OK;
  reg_16_ptr_t dest = reg_16_select(state, operands->dest);
  if (dest != NULL)
  {
    --(*dest);
  }
  return status;
}

status_code_t op_ADD_8(cpu_state_t *const state, inst_operands_t *const operands)
{

  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);
  uint8_t half_carry = ((*dest & 0x0F) + (data & 0x0F)) & 0x10;
  uint16_t full_carry = ((*dest & 0xFF) + (data & 0xFF)) & 0x100;

  *dest += data;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_ADC_8(cpu_state_t *const state, inst_operands_t *const operands)
{

  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);
  uint8_t carry_bit = (state->registers.f & FLAG_C) ? 1 : 0;
  uint8_t half_carry = ((*dest & 0x0F) + (data & 0x0F) + carry_bit) & 0x10;
  uint16_t full_carry = ((*dest & 0xFF) + (data & 0xFF) + carry_bit) & 0x100;

  *dest += data + carry_bit;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_SUB_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);
  uint8_t half_carry = ((*dest & 0x0F) < (data & 0x0F)) ? 1 : 0;
  uint8_t full_carry = (*dest < data) ? 1 : 0;

  *dest -= data;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_SET);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_SBC_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);
  uint8_t carry_bit = (state->registers.f & FLAG_C) ? 1 : 0;
  uint8_t half_carry = ((*dest & 0x0F) < ((data & 0x0F) + carry_bit)) ? 1 : 0;
  uint16_t full_carry = (*dest < (data + carry_bit)) ? 1 : 0;

  *dest -= data;
  *dest -= carry_bit;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_SET);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_AND_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);

  *dest &= data;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_H, F_SET);
  update_flags(state, FLAG_N | FLAG_C, F_CLEAR);

  return STATUS_OK;
}

status_code_t op_XOR_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);

  *dest ^= data;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H | FLAG_C, F_CLEAR);

  return STATUS_OK;
}

status_code_t op_OR_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);

  *dest |= data;

  update_flags(state, FLAG_Z, *dest == 0 ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H | FLAG_C, F_CLEAR);

  return STATUS_OK;
}

status_code_t op_CP_8(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, operands->source, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  reg_8_ptr_t dest = reg_8_select(state, operands->dest);
  uint8_t half_carry = ((*dest & 0x0F) < (data & 0x0F)) ? 1 : 0;
  uint8_t full_carry = (*dest < data) ? 1 : 0;

  update_flags(state, FLAG_Z, (*dest == data) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N, F_SET);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_ADD_16(cpu_state_t *const state, inst_operands_t *const operands)
{

  reg_16_ptr_t source = reg_16_select(state, operands->source);
  reg_16_ptr_t dest = reg_16_select(state, operands->dest);

  if (operands->source == AM_IMM_S_8)
  {
    return read_reg_16_plus_offset(state, operands->dest, dest);
  }

  uint16_t half_carry = ((*dest & 0x0FFF) + (*source & 0x0FFF)) & 0x1000;
  uint32_t full_carry = ((*dest & 0xFFFF) + (*source & 0xFFFF)) & 0x10000;

  *dest += *source;

  update_flags(state, FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, half_carry ? F_SET : F_CLEAR);
  update_flags(state, FLAG_C, full_carry ? F_SET : F_CLEAR);

  return STATUS_OK;
}

status_code_t op_RLCA(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;

  uint8_t msb = (regs->a & (1 << 7)) ? 1 : 0;
  regs->a = (regs->a << 1) | msb;

  update_flags(state, FLAG_Z | FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, msb ? F_SET : F_CLEAR);

  return STATUS_OK;
}

status_code_t op_RRCA(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;

  uint8_t lsb = regs->a & 0x1;
  regs->a = (regs->a >> 1) | (lsb << 7);

  update_flags(state, FLAG_Z | FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return STATUS_OK;
}

status_code_t op_RLA(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;

  uint8_t msb = (regs->a & (1 << 7)) ? 1 : 0;
  regs->a = (regs->a << 1);
  regs->a |= ((regs->f & FLAG_C) ? 1 : 0);

  update_flags(state, FLAG_Z | FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, msb ? F_SET : F_CLEAR);

  return STATUS_OK;
}

status_code_t op_RRA(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;

  uint8_t lsb = regs->a & 0x1;
  regs->a = (regs->a >> 1);
  regs->a |= ((regs->f & FLAG_C) ? (1 << 7) : 0);

  update_flags(state, FLAG_Z | FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return STATUS_OK;
}

status_code_t op_CPL(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;
  regs->a = ~(regs->a);
  update_flags(state, FLAG_N | FLAG_H, F_SET);
  return STATUS_OK;
}

status_code_t op_SCF(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, F_SET);
  return STATUS_OK;
}

status_code_t op_CCF(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, F_INVERT);
  return STATUS_OK;
}

status_code_t op_RET(cpu_state_t *const state, inst_operands_t *const operands)
{
  if (check_cond(state, operands->jump_mode))
  {
    return pop_reg_16(state, &state->registers.pc);
  }
  return STATUS_OK;
}

status_code_t op_CALL(cpu_state_t *const state, inst_operands_t *const operands)
{
  uint16_t address;
  status_code_t status = STATUS_OK;

  status = read_16(state, operands->source, &address);
  RETURN_STATUS_IF_NOT_OK(status);

  if (check_cond(state, operands->jump_mode))
  {
    status = push_reg_16(state, &state->registers.pc);
    state->registers.pc = address;
  }

  return status;
}

status_code_t op_POP(cpu_state_t *const state, inst_operands_t *const operands)
{
  return pop_reg_16(state, reg_16_select(state, operands->dest));
}

status_code_t op_PUSH(cpu_state_t *const state, inst_operands_t *const operands)
{
  return push_reg_16(state, reg_16_select(state, operands->dest));
}

status_code_t op_RST(cpu_state_t *const state, inst_operands_t *const operands)
{
  status_code_t status = STATUS_OK;
  status = push_reg_16(state, &state->registers.pc);
  state->registers.pc = operands->reset_vector;
  return status;
}

status_code_t op_DAA(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  registers_t *const regs = &state->registers;

  if (!(regs->f & FLAG_N))
  {
    if ((regs->f & FLAG_C) || (regs->a > 0x99))
    {
      regs->a += 0x60;
      update_flags(state, FLAG_C, F_SET);
    }
    if ((regs->f & FLAG_H) || ((regs->a & 0x0F) > 0x09))
    {
      regs->a += 0x06;
    }
  }
  else
  {
    if (regs->f & FLAG_C)
    {
      regs->a -= 0x60;
    }
    if (regs->f & FLAG_H)
    {
      regs->a -= 0x06;
    }
  }

  update_flags(state, FLAG_Z, (regs->a) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_H, F_SET);

  return STATUS_OK;
}

status_code_t op_RETI(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  state->ime_flag = 1;
  return pop_reg_16(state, &state->registers.pc);
}

status_code_t op_DI(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  state->ime_flag = 0;
  state->next_ime_flag = 0;
  return STATUS_OK;
}

status_code_t op_EI(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  state->next_ime_flag = 1;
  return STATUS_OK;
}

status_code_t op_PRCB(cpu_state_t *const state, inst_operands_t *const __attribute__((unused)) operands)
{
  static addressing_mode_t const target_lookup_table[] = {
      AM_REG_B,
      AM_REG_C,
      AM_REG_D,
      AM_REG_E,
      AM_REG_H,
      AM_REG_L,
      AM_MEM_HL,
      AM_REG_A,
  };

  static cb_opcode_handler_t const handlers[] = {
      {op_RLC},
      {op_RRC},
      {op_RL},
      {op_RR},
      {op_SLA},
      {op_SRA},
      {op_SWAP},
      {op_SRL},
  };

  uint8_t cb_opcode;
  status_code_t status = STATUS_OK;
  registers_t *const regs = &state->registers;

  status = bus_read_8(state, regs->pc++, &cb_opcode);
  RETURN_STATUS_IF_NOT_OK(status);

  uint8_t target_type = (cb_opcode & 0x7);
  uint8_t bit_index = ((cb_opcode >> 3) & 0x7);
  uint8_t handler_family = ((cb_opcode >> 6) & 0x3);

  addressing_mode_t target = target_lookup_table[target_type];

  switch (handler_family)
  {
  case 0:
    status = handlers[bit_index].rot_shf_swap_handler_fn(state, target);
    break;
  case 1:
    status = op_BIT(state, target, bit_index);
    break;
  case 2:
    status = op_RES(state, target, bit_index);
    break;
  case 3:
    status = op_SET(state, target, bit_index);
    break;
  default:
    break;
  }

  state->m_cycles += (target == AM_MEM_HL) ? ((handler_family == 1) ? 3 : 4) : 2;

  return status;
}

/* Ops for CB-prefixed opcodes */
status_code_t op_RLC(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, msb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  msb = data & (1 << 7) ? 1 : 0;
  data <<= 1;
  data |= msb ? 1 : 0;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, msb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_RRC(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, lsb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  lsb = data & 0x1;
  data >>= 1;
  data |= lsb ? (1 << 7) : 0;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_RL(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, msb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  msb = data & (1 << 7) ? 1 : 0;
  data <<= 1;
  data |= (state->registers.f & FLAG_C) ? 1 : 0;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, msb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_RR(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, lsb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  lsb = data & 0x1;
  data >>= 1;
  data |= (state->registers.f & FLAG_C) ? (1 << 7) : 0;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_SLA(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, msb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  msb = (data & (1 << 7)) ? 1 : 0;
  data <<= 1;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, msb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_SRA(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, msb, lsb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  msb = data & (1 << 7);
  lsb = data & 0x1;
  data = msb | (data >> 1);

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_SWAP(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  data = ((data & 0xF) << 4) | ((data >> 4) & 0xF);
  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H | FLAG_C, F_CLEAR);

  return status;
}

status_code_t op_SRL(cpu_state_t *const state, addressing_mode_t const addr_mode)
{
  uint8_t data, lsb;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  lsb = data & 0x1;
  data >>= 1;

  status = write_8(state, addr_mode, data);

  update_flags(state, FLAG_Z, (data == 0) ? F_SET : F_CLEAR);
  update_flags(state, FLAG_N | FLAG_H, F_CLEAR);
  update_flags(state, FLAG_C, lsb ? F_SET : F_CLEAR);

  return status;
}

status_code_t op_BIT(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  update_flags(state, FLAG_Z, (data & (1 << bit_index)) ? F_CLEAR : F_SET);
  update_flags(state, FLAG_N, F_CLEAR);
  update_flags(state, FLAG_H, F_SET);

  return STATUS_OK;
}

status_code_t op_RES(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  return write_8(state, addr_mode, (data & ~(1 << bit_index)));
}

status_code_t op_SET(cpu_state_t *const state, addressing_mode_t const addr_mode, uint8_t const bit_index)
{
  uint8_t data;
  status_code_t status = STATUS_OK;

  status = read_8(state, addr_mode, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  return write_8(state, addr_mode, (data | (1 << bit_index)));
}
