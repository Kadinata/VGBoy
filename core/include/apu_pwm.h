#ifndef __DMG_APU_PWM_H__
#define __DMG_APU_PWM_H__

#include <stdint.h>
#include <stdbool.h>

#include "bus_interface.h"
#include "status_code.h"

typedef enum
{
  APU_SWEEP_STEP = 0x07,
  APU_SWEEP_DIR = 0x08,
  APU_SWEEP_PACE = 0x70,
} apu_sweep_mask_t;

typedef enum
{
  APU_TMRD_WAVE_DUTY = 0xC0,
} apu_tmrd_mask_t;

typedef enum
{
  APU_PERIOD_LSB = 0xFF,
  APU_PERIOD_MSB = 0x07,
} apu_pwm_period_mask_t;

typedef struct __attribute__((packed))
{
  uint8_t sweep;  /* NRx0: Sweep */
  uint8_t tmrd;   /* NRx1: Length timer & duty cycle */
  uint8_t volenv; /* NRx2: Volume & envelope */
  uint8_t plow;   /* NRx3: Period low [write-only] */
  uint8_t phctl;  /* NRx4: Period high & control */
} apu_pwm_registers_t;

typedef struct __attribute__((packed))
{
  bool available;
  bool enabled;
  uint8_t timer;
  uint16_t shadow_freq;
} apu_pwm_sweep_state_t;

typedef struct
{
  bool enabled;
  uint8_t wave_duty_position;
  uint16_t period_counter;
  uint8_t length_timer;
  uint8_t envelope_timer;
  uint8_t volume;
  apu_pwm_sweep_state_t sweep;
} apu_pwm_state_t;

typedef struct
{
  apu_pwm_state_t state;
  apu_pwm_registers_t registers;
  bus_interface_t bus_interface;
} apu_pwm_handle_t;

status_code_t apu_pwm_init(apu_pwm_handle_t *const apu_pwm, bool const with_sweep);
status_code_t apu_pwm_tick(apu_pwm_handle_t *const apu_pwm);
status_code_t apu_pwm_reset(apu_pwm_handle_t *const apu_pwm);
status_code_t apu_pwm_sample(apu_pwm_handle_t *const apu_pwm, float *const sample_out);
status_code_t apu_pwm_handle_frame_sequencer(apu_pwm_handle_t *const apu_pwm, uint8_t const frame_step);

#endif /* __DMG_APU_PWM_H__ */
