#ifndef __DMG_SYSTEM_DEF_H__
#define __DMG_SYSTEM_DEF_H__

#include <stdint.h>

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
} cpu_runmode_t;

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
  cpu_runmode_t run_mode;
  uint8_t ime_flag;
} cpu_state_t;

#endif /* __DMG_SYSTEM_DEF_H__ */
