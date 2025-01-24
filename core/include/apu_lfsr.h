#ifndef __DMG_APU_LFSR_H__
#define __DMG_APU_LFSR_H__

#include <stdint.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "status_code.h"

typedef enum
{
  APU_FRQRAND_CLK_DIV = 0x07,
  APU_FRQRAND_LFSR_WIDTH = 0x08,
  APU_FRQRAND_CLK_SHIFT = 0xF0,
} apu_lfsr_reg_mask_t;

typedef struct __attribute__((packed))
{
  uint8_t _unused; /* 0xFF1F - Unused */
  uint8_t ltmr;    /* 0xFF20 — NR41: Channel 4 length timer [write-only] */
  uint8_t volenv;  /* 0xFF21 — NR42: Channel 4 volume & envelope */
  uint8_t frqrand; /* 0xFF22 — NR43: Channel 4 frequency & randomness */
  uint8_t ctrl;    /* 0xFF23 — NR44: Channel 4 control */
} apu_lfsr_registers_t;

typedef struct
{
  bool enabled;
  uint16_t period_counter;
  uint8_t length_timer;
  uint8_t envelope_timer;
  uint8_t volume;
  uint16_t lfsr;
} apu_lfsr_state_t;

typedef struct
{
  apu_lfsr_state_t state;
  apu_lfsr_registers_t registers;
  bus_interface_t bus_interface;
} apu_lfsr_handle_t;

status_code_t apu_lfsr_init(apu_lfsr_handle_t *const apu_lfsr);
status_code_t apu_lfsr_tick(apu_lfsr_handle_t *const apu_lfsr);
status_code_t apu_lfsr_sample(apu_lfsr_handle_t *const apu_lfsr, float *const sample_out);
status_code_t apu_lfsr_handle_frame_sequencer(apu_lfsr_handle_t *const apu_lfsr, uint8_t const frame_step);

#endif /* __DMG_APU_LFSR_H__ */
