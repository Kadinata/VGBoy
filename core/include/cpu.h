#ifndef __DMG_CPU_H__
#define __DMG_CPU_H__

#include <stdint.h>

#include "bus_interface.h"
#include "callback.h"
#include "interrupt.h"
#include "status_code.h"

/* Address of entry point */
#define ENTRY_PT_ADDR (0x100)

/* Flag register bit-masks */
#define FLAG_Z (1 << 7) // Zero flag
#define FLAG_N (1 << 6) // Subtraction flag (BCD)
#define FLAG_H (1 << 5) // Half-carry flag (BCD)
#define FLAG_C (1 << 4) // Carry flag

typedef enum
{
  RUN_MODE_NORMAL,
  RUN_MODE_STOPPED,
  RUN_MODE_HALTED,
} cpu_run_mode_t;

/**
 * CPU register definitions. There are 8 8-bit data registers:
 * A, B, C, D, E, F, H, and L. These registers can be paired
 * as 16-bit registers: AF, BC, DE, HL. Register A is the
 * accumulator, and register F is the flag register
 */
typedef struct registers_s
{
  union
  {
    /* 8 x 8-bit data registers */
    struct __attribute__((packed))
    {
      uint8_t f;
      uint8_t a;
      uint8_t c;
      uint8_t b;
      uint8_t e;
      uint8_t d;
      uint8_t l;
      uint8_t h;
    };

    /* Paired data registers as 4 x 16-bit registers */
    struct __attribute__((packed))
    {
      uint16_t af;
      uint16_t bc;
      uint16_t de;
      uint16_t hl;
    };
  };

  /* 16-bit program counter */
  uint16_t pc;

  /* 16-bit stack pointer */
  uint16_t sp;
} registers_t;

/* CPU state definition */
typedef struct cpu_state_s
{
  registers_t registers;
  uint32_t m_cycles;
  cpu_run_mode_t run_mode;
  uint8_t next_ime_flag;
  interrupt_handle_t interrupt;
  callback_t interrupt_callback;
  bus_interface_t bus_interface;
  callback_t *cycle_sync_callback;
  uint8_t current_inst_m_cycle_count; // TODO: find more elegant solution
} cpu_state_t;

typedef struct
{
  bus_interface_t *bus_interface;
  callback_t *cycle_sync_callback;
} cpu_init_param_t;

status_code_t cpu_init(cpu_state_t *const state, cpu_init_param_t *const param);
status_code_t cpu_emulation_cycle(cpu_state_t *const state);

#endif /* __DMG_CPU_H__ */
