#include "apu_pwm.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "apu_common.h"
#include "bus_interface.h"
#include "status_code.h"

static status_code_t apu_pwm_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_pwm_write(void *const resource, uint16_t const address, uint8_t const data);

static status_code_t update_envelope(apu_pwm_handle_t *const apu_pwm);
static status_code_t update_sweep(apu_pwm_handle_t *const apu_pwm);
static uint16_t calculate_new_sweep_freq(apu_pwm_handle_t *const apu_pwm);
static inline uint16_t get_channel_period(apu_pwm_handle_t *const apu_pwm);
static inline uint8_t get_current_amplitude(apu_pwm_handle_t *const apu_pwm);
static inline uint8_t get_sweep_pace(apu_pwm_handle_t *const apu_pwm);
static inline uint8_t get_sweep_step(apu_pwm_handle_t *const apu_pwm);
static inline void trigger_channel(apu_pwm_handle_t *const apu_pwm);
static inline bool length_timer_enabled(apu_pwm_handle_t *const apu_pwm);

static const uint8_t apu_pwm_wave_duty[] = {
    0x01, /* 12.5%: 00000001 */
    0x03, /* 25%  : 00000011 */
    0x0F, /* 50%  : 00001111 */
    0xFC, /* 75%  : 11111100 */
};

status_code_t apu_pwm_init(apu_pwm_handle_t *const apu_pwm, bool const with_sweep)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_pwm);

  status_code_t status = STATUS_OK;

  status = bus_interface_init(&apu_pwm->bus_interface, apu_pwm_read, apu_pwm_write, apu_pwm);
  RETURN_STATUS_IF_NOT_OK(status);

  apu_pwm->state.sweep.available = with_sweep;

  return STATUS_OK;
}

status_code_t apu_pwm_tick(apu_pwm_handle_t *const apu_pwm)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_pwm);

  if (apu_pwm->state.period_counter == 0)
  {
    apu_pwm->state.period_counter = 2048 - get_channel_period(apu_pwm);
    apu_pwm->state.wave_duty_position++;
    apu_pwm->state.wave_duty_position &= 0x7;
  }

  apu_pwm->state.period_counter--;
  return STATUS_OK;
}

status_code_t apu_pwm_sample(apu_pwm_handle_t *const apu_pwm, float *const sample_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_pwm);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(sample_out);

  *sample_out = apu_pwm->state.enabled ? (get_current_amplitude(apu_pwm) * (apu_pwm->state.volume / 15.0f)) : 0.0f;
  return STATUS_OK;
}

status_code_t apu_pwm_handle_frame_sequencer(apu_pwm_handle_t *const apu_pwm, uint8_t const frame_step)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_pwm);

  if (((frame_step & 0x1) == 0) && length_timer_enabled(apu_pwm) && apu_pwm->state.length_timer)
  {
    apu_pwm->state.length_timer--;
    apu_pwm->state.enabled = (apu_pwm->state.length_timer > 0);
  }

  if (apu_pwm->state.sweep.available && ((frame_step == 2) || (frame_step == 6)))
  {
    return update_sweep(apu_pwm);
  }

  if (frame_step == 0x7)
  {
    return update_envelope(apu_pwm);
  }

  return STATUS_OK;
}

static status_code_t update_envelope(apu_pwm_handle_t *const apu_pwm)
{
  const uint8_t envelope_pace = apu_pwm->registers.volenv & APU_ENVELOPE_PACE;
  const uint8_t envelope_dir = apu_pwm->registers.volenv & APU_ENVELOPE_DIR;

  if ((envelope_pace == 0) || (apu_pwm->state.envelope_timer == 0))
  {
    return STATUS_OK;
  }

  apu_pwm->state.envelope_timer--;

  if (apu_pwm->state.envelope_timer == 0)
  {
    apu_pwm->state.envelope_timer = envelope_pace;

    if (envelope_dir && (apu_pwm->state.volume < 0xF))
    {
      apu_pwm->state.volume++;
    }
    else if (!envelope_dir && (apu_pwm->state.volume > 0x0))
    {
      apu_pwm->state.volume--;
    }
  }

  return STATUS_OK;
}

static status_code_t update_sweep(apu_pwm_handle_t *const apu_pwm)
{
  if (apu_pwm->state.sweep.timer)
  {
    apu_pwm->state.sweep.timer--;
  }

  if (apu_pwm->state.sweep.timer)
  {
    return STATUS_OK;
  }

  uint8_t sweep_pace = get_sweep_pace(apu_pwm);
  apu_pwm->state.sweep.timer = sweep_pace ? sweep_pace : 8;

  if (!apu_pwm->state.sweep.enabled || (sweep_pace == 0))
  {
    return STATUS_OK;
  }

  uint16_t new_freq = calculate_new_sweep_freq(apu_pwm);
  uint8_t sweep_step = get_sweep_step(apu_pwm);

  if ((new_freq <= 2047) && sweep_step)
  {
    apu_pwm->registers.plow = (new_freq & APU_PERIOD_LSB);
    apu_pwm->registers.phctl = (apu_pwm->registers.phctl & ~APU_PERIOD_MSB) | ((new_freq >> 8) & APU_PERIOD_MSB);
    apu_pwm->state.sweep.shadow_freq = new_freq;

    /* for overflow check */
    calculate_new_sweep_freq(apu_pwm);
  }

  return STATUS_OK;
}

static uint16_t calculate_new_sweep_freq(apu_pwm_handle_t *const apu_pwm)
{
  uint8_t sweep_step = get_sweep_step(apu_pwm);
  uint16_t new_frequency = apu_pwm->state.sweep.shadow_freq >> sweep_step;

  if (apu_pwm->registers.sweep & APU_SWEEP_DIR)
  {
    new_frequency -= apu_pwm->state.sweep.shadow_freq;
  }
  else
  {
    new_frequency += apu_pwm->state.sweep.shadow_freq;
  }

  if (new_frequency > 2047)
  {
    apu_pwm->state.enabled = false;
  }

  return new_frequency;
}

static inline uint16_t get_channel_period(apu_pwm_handle_t *const apu_pwm)
{
  return (apu_pwm == NULL) ? 0 : (((apu_pwm->registers.phctl & APU_PERIOD_MSB) << 8) | apu_pwm->registers.plow);
}

static inline uint8_t get_current_amplitude(apu_pwm_handle_t *const apu_pwm)
{
  if (!apu_pwm)
  {
    return 0;
  }
  uint8_t wave_duty_index = (apu_pwm->registers.tmrd & APU_TMRD_WAVE_DUTY) >> 6;
  return (apu_pwm_wave_duty[wave_duty_index] & (1 << apu_pwm->state.wave_duty_position)) ? 1 : 0;
}

static inline uint8_t get_sweep_pace(apu_pwm_handle_t *const apu_pwm)
{
  return (apu_pwm->registers.sweep & APU_SWEEP_PACE) >> 4;
}

static inline uint8_t get_sweep_step(apu_pwm_handle_t *const apu_pwm)
{
  return apu_pwm->registers.sweep & APU_SWEEP_STEP;
}

static inline bool length_timer_enabled(apu_pwm_handle_t *const apu_pwm)
{
  return !!(apu_pwm->registers.phctl & APU_LENGTH_EN);
}

static inline void trigger_channel(apu_pwm_handle_t *const apu_pwm)
{
  apu_pwm->state.enabled = !!(apu_pwm->registers.volenv & APU_CHANNEL_ENABLED);
  apu_pwm->state.wave_duty_position = 0;
  apu_pwm->state.volume = (apu_pwm->registers.volenv & APU_START_VOLUME) >> 4;
  apu_pwm->state.length_timer = apu_pwm->state.length_timer ? apu_pwm->state.length_timer : 64;
  apu_pwm->state.envelope_timer = apu_pwm->registers.volenv & APU_ENVELOPE_PACE;

  if (!apu_pwm->state.sweep.available)
  {
    return;
  }

  uint8_t sweep_pace = get_sweep_pace(apu_pwm);
  uint8_t sweep_step = get_sweep_step(apu_pwm);

  apu_pwm->state.sweep.shadow_freq = get_channel_period(apu_pwm);
  apu_pwm->state.sweep.timer = sweep_pace ? sweep_pace : 8;
  apu_pwm->state.sweep.enabled = (sweep_pace && sweep_step);

  if (sweep_step)
  {
    calculate_new_sweep_freq(apu_pwm);
  }
}

static status_code_t apu_pwm_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  static const uint8_t read_masks[] = {0x80, 0x3F, 0x00, 0xFF, 0xBF};

  apu_pwm_handle_t *const apu_pwm = (apu_pwm_handle_t *)resource;

  switch (address)
  {
  case 0x0000:
    *data = apu_pwm->state.sweep.available ? apu_pwm->registers.sweep : 0xFF;
    break;
  case 0x0001:
    *data = apu_pwm->registers.tmrd;
    break;
  case 0x0002:
    *data = apu_pwm->registers.volenv;
    break;
  case 0x0003:
    *data = 0xFF;
    break;
  case 0x0004:
    *data = apu_pwm->registers.phctl;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  *data |= read_masks[address];

  return STATUS_OK;
}

static status_code_t apu_pwm_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  apu_pwm_handle_t *const apu_pwm = (apu_pwm_handle_t *)resource;

  switch (address)
  {
  case 0x0000:
    apu_pwm->registers.sweep = apu_pwm->state.sweep.available ? data : 0x00;
    break;
  case 0x0001:
    apu_pwm->registers.tmrd = data;
    apu_pwm->state.length_timer = 64 - (data & APU_LENGTH_TIMER);
    break;
  case 0x0002:
    apu_pwm->registers.volenv = data;
    apu_pwm->state.enabled = !!(data & APU_CHANNEL_ENABLED);
    break;
  case 0x0003:
    apu_pwm->registers.plow = data;
    break;
  case 0x0004:
    apu_pwm->registers.phctl = data;
    if (data & APU_TRIGGER_EN)
    {
      trigger_channel(apu_pwm);
    }
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}
