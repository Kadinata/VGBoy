#ifndef __DMG_APU_H__
#define __DMG_APU_H__

#include <stdint.h>

#include "apu_pwm.h"
#include "apu_lfsr.h"
#include "apu_wave.h"
#include "bus_interface.h"
#include "callback.h"
#include "status_code.h"

typedef enum
{
  APU_ACTL_CH1_EN = (1 << 0),
  APU_ACTL_CH2_EN = (1 << 1),
  APU_ACTL_CH3_EN = (1 << 2),
  APU_ACTL_CH4_EN = (1 << 3),
  APU_ACTL_AUDIO_EN = (1 << 7),
} apu_actl_t;

typedef enum
{
  APU_SNDP_CH1_RIGHT = (1 << 0),
  APU_SNDP_CH2_RIGHT = (1 << 1),
  APU_SNDP_CH3_RIGHT = (1 << 2),
  APU_SNDP_CH4_RIGHT = (1 << 3),
  APU_SNDP_CH1_LEFT = (1 << 4),
  APU_SNDP_CH2_LEFT = (1 << 5),
  APU_SNDP_CH3_LEFT = (1 << 6),
  APU_SNDP_CH4_LEFT = (1 << 7),
} apu_sndp_t;

typedef enum
{
  APU_MVP_RIGHT_VOL = (0x7),
  APU_MVP_RIGHT_VIN = (1 << 4),
  APU_MVP_LEFT_VOL = (0x70),
  APU_MVP_LEFT_VIN = (1 << 7),
} apu_mvp_t;

typedef struct __attribute__((packed))
{
  uint8_t mvp;  /* 0xFF24 — NR50: Master volume & VIN panning */
  uint8_t sndp; /* 0xFF25 — NR51: Sound panning */
  uint8_t actl; /* 0xFF26 — NR52: Audio master control */
} apu_registers_t;

typedef struct
{
  uint16_t tick_count;
  uint8_t frame_step;
} apu_frame_sequencer_counter_t;

typedef struct
{
  apu_registers_t registers;
  apu_pwm_handle_t ch1;
  apu_pwm_handle_t ch2;
  apu_wave_handle_t ch3;
  apu_lfsr_handle_t ch4;
  apu_frame_sequencer_counter_t frame_sequencer;
  bus_interface_t bus_interface;
  callback_t playback_cb;
} apu_handle_t;

status_code_t apu_init(apu_handle_t *const apu);
status_code_t apu_tick(apu_handle_t *const apu);

#endif /* __DMG_APU_H__ */
