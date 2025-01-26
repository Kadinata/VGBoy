#ifndef __DMG_APU_WAVE_H__
#define __DMG_APU_WAVE_H__

#include <stdint.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "status_code.h"

typedef enum
{
  /* NR30 Masks */
  APU_WAVE_DAC_EN = 0x80,

  /* NR32 Masks */
  APU_WAVE_VOL = 0x60,

  /* NR33 Masks */
  APU_WAVE_PERIOD_LSB = 0xFF,

  /* NR34 Masks */
  APU_WAVE_PERIOD_MSB = 0x07,
} apu_wave_reg_mask_t;

typedef struct __attribute__((packed))
{
  uint8_t dacen; /* 0xFF1A — NR30: Channel 3 DAC enable */
  uint8_t ltmr;  /* 0xFF1B — NR31: Channel 3 length timer [write-only] */
  uint8_t vol;   /* 0xFF1C — NR32: Channel 3 output level */
  uint8_t plow;  /* 0xFF1D — NR33: Channel 3 period low [write-only] */
  uint8_t phctl; /* 0xFF1E — NR34: Channel 3 period high & control */
} apu_wave_registers_t;

typedef struct __attribute__((packed))
{
  bool enabled;
  uint8_t envelope_timer;
  uint8_t volume;
  uint8_t sample_index;
  uint16_t length_timer;
  uint16_t period_timer;
} apu_wave_state_t;

typedef struct
{
  uint8_t data[16];
  bus_interface_t bus_interface;
} apu_wave_ram_t;

typedef struct
{
  apu_wave_state_t state;
  apu_wave_registers_t registers;
  apu_wave_ram_t wave_ram;
  bus_interface_t bus_interface;
} apu_wave_handle_t;

status_code_t apu_wave_init(apu_wave_handle_t *const apu_wave);
status_code_t apu_wave_tick(apu_wave_handle_t *const apu_wave);
status_code_t apu_wave_reset(apu_wave_handle_t *const apu_wave);
status_code_t apu_wave_sample(apu_wave_handle_t *const apu_wave, float *const sample_out);
status_code_t apu_wave_handle_frame_sequencer(apu_wave_handle_t *const apu_wave, uint8_t const frame_step);

#endif /*  __DMG_APU_WAVE_H__ */
