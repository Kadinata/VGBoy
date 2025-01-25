#include "apu.h"

#include <stdint.h>
#include <string.h>

#include "apu_pwm.h"
#include "apu_lfsr.h"
#include "apu_wave.h"
#include "audio_playback_samples.h"
#include "bus_interface.h"
#include "callback.h"
#include "logging.h"
#include "status_code.h"

#define CPU_FREQ (1048576)
#define DEFAULT_VOLUME (INT16_MAX)

static status_code_t apu_bus_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_bus_write(void *const resource, uint16_t const address, uint8_t const data);
static status_code_t apu_sample(apu_handle_t *const apu, float *const left_sample, float *const right_sample);
static status_code_t apu_playback(void *const ctx, const void *arg);

status_code_t apu_init(apu_handle_t *const apu)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu);
  status_code_t status = STATUS_OK;

  apu->frame_sequencer.tick_count = 0;
  apu->frame_sequencer.frame_step = 0;
  apu->ch1.bus_interface.offset = 0x0000;
  apu->ch2.bus_interface.offset = 0x0005;
  apu->ch3.bus_interface.offset = 0x000A;
  apu->ch4.bus_interface.offset = 0x000F;
  apu->ch3.wave_ram.bus_interface.offset = 0x0020;

  status = apu_pwm_init(&apu->ch1, true);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_pwm_init(&apu->ch2, false);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_wave_init(&apu->ch3);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_lfsr_init(&apu->ch4);
  RETURN_STATUS_IF_NOT_OK(status);

  status = callback_init(&apu->playback_cb, apu_playback, apu);
  RETURN_STATUS_IF_NOT_OK(status);

  return bus_interface_init(&apu->bus_interface, apu_bus_read, apu_bus_write, apu);
}

status_code_t apu_tick(apu_handle_t *const apu)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu);

  apu_pwm_tick(&apu->ch1);
  apu_pwm_tick(&apu->ch2);
  apu_wave_tick(&apu->ch3);
  apu_lfsr_tick(&apu->ch4);

  apu->frame_sequencer.tick_count++;

  if (apu->frame_sequencer.tick_count == (8192 / 4))
  {
    apu_pwm_handle_frame_sequencer(&apu->ch1, apu->frame_sequencer.frame_step);
    apu_pwm_handle_frame_sequencer(&apu->ch2, apu->frame_sequencer.frame_step);
    apu_wave_handle_frame_sequencer(&apu->ch3, apu->frame_sequencer.frame_step);
    apu_lfsr_handle_frame_sequencer(&apu->ch4, apu->frame_sequencer.frame_step);

    apu->frame_sequencer.tick_count = 0;
    apu->frame_sequencer.frame_step++;
    apu->frame_sequencer.frame_step &= 0x7;
  }

  return STATUS_OK;
}

static status_code_t apu_sample(apu_handle_t *const apu, float *const left_sample, float *const right_sample)
{
  float channel_samples[4];

  status_code_t status = STATUS_OK;

  float left_volume = ((apu->registers.mvp & APU_MVP_LEFT_VOL) >> 4) + 1;
  float right_volume = (apu->registers.mvp & APU_MVP_RIGHT_VOL) + 1;

  status = apu_pwm_sample(&apu->ch1, &channel_samples[0]);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_pwm_sample(&apu->ch2, &channel_samples[1]);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_wave_sample(&apu->ch3, &channel_samples[2]);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_lfsr_sample(&apu->ch4, &channel_samples[3]);
  RETURN_STATUS_IF_NOT_OK(status);

  *left_sample = 0.0f;
  *right_sample = 0.0f;

  *left_sample += (apu->registers.sndp & APU_SNDP_CH1_LEFT) ? channel_samples[0] : 0;
  *left_sample += (apu->registers.sndp & APU_SNDP_CH2_LEFT) ? channel_samples[1] : 0;
  *left_sample += (apu->registers.sndp & APU_SNDP_CH3_LEFT) ? channel_samples[2] : 0;
  *left_sample += (apu->registers.sndp & APU_SNDP_CH4_LEFT) ? channel_samples[3] : 0;

  *right_sample += (apu->registers.sndp & APU_SNDP_CH1_RIGHT) ? channel_samples[0] : 0;
  *right_sample += (apu->registers.sndp & APU_SNDP_CH2_RIGHT) ? channel_samples[1] : 0;
  *right_sample += (apu->registers.sndp & APU_SNDP_CH3_RIGHT) ? channel_samples[2] : 0;
  *right_sample += (apu->registers.sndp & APU_SNDP_CH4_RIGHT) ? channel_samples[3] : 0;

  *left_sample /= 4.0f;
  *right_sample /= 4.0f;

  *left_sample *= (left_volume / 8.0f);
  *right_sample *= (right_volume / 8.0f);

  return STATUS_OK;
}

static status_code_t apu_playback(void *const ctx, const void *arg)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ctx);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(arg);

  apu_handle_t *const apu = (apu_handle_t *)ctx;
  audio_playback_samples_t *const playback_samples = (audio_playback_samples_t *)arg;

  int16_t *stream_buf = (int16_t *)playback_samples->data;
  uint32_t buffer_len = playback_samples->length / sizeof(int16_t);

  float left_sample = 0.0f;
  float right_sample = 0.0f;

  memset(playback_samples->data, 0, playback_samples->length);

  const uint32_t ticks_per_sample = CPU_FREQ / playback_samples->sample_rate_hz;

  for (uint32_t i = 0; i < buffer_len; i += 2)
  {
    for (uint32_t j = 0; j < ticks_per_sample; j++)
    {
      apu_tick(apu);
    }

    if (apu->registers.actl & APU_ACTL_AUDIO_EN)
    {
      apu_sample(apu, &left_sample, &right_sample);

      left_sample *= playback_samples->volume_adjust * DEFAULT_VOLUME;
      right_sample *= playback_samples->volume_adjust * DEFAULT_VOLUME;

      stream_buf[i] = (int16_t)(left_sample);
      stream_buf[i + 1] = (int16_t)(right_sample);
    }
  }

  return STATUS_OK;
}

static status_code_t apu_bus_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  apu_handle_t *const apu = (apu_handle_t *)resource;
  status_code_t status = STATUS_OK;

  *data = 0xFF;

  if ((address >= 0x0000) && (address < 0x0005))
  {
    status = bus_interface_read(&apu->ch1.bus_interface, address, data);
  }
  else if ((address >= 0x0005) && (address < 0x000A))
  {
    status = bus_interface_read(&apu->ch2.bus_interface, address, data);
  }
  else if ((address >= 0x000A) && (address < 0x000F))
  {
    status = bus_interface_read(&apu->ch3.bus_interface, address, data);
  }
  else if ((address >= 0x000F) && (address < 0x0014))
  {
    status = bus_interface_read(&apu->ch4.bus_interface, address, data);
  }
  else if (address == 0x0014)
  {
    *data = apu->registers.mvp;
  }
  else if (address == 0x0015)
  {
    *data = apu->registers.sndp;
  }
  else if (address == 0x0016)
  {
    *data = apu->registers.actl & APU_ACTL_AUDIO_EN;
    *data |= apu->ch1.state.enabled ? APU_ACTL_CH1_EN : 0;
    *data |= apu->ch2.state.enabled ? APU_ACTL_CH2_EN : 0;
    *data |= apu->ch3.state.enabled ? APU_ACTL_CH3_EN : 0;
    *data |= apu->ch4.state.enabled ? APU_ACTL_CH4_EN : 0;
  }
  else if ((address >= 0x0020) && (address < 0x0030))
  {
    status = bus_interface_read(&apu->ch3.wave_ram.bus_interface, address, data);
  }
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t apu_bus_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  apu_handle_t *const apu = (apu_handle_t *)resource;
  status_code_t status = STATUS_OK;

  if ((address >= 0x0000) && (address < 0x0005))
  {
    status = bus_interface_write(&apu->ch1.bus_interface, address, data);
  }
  else if ((address >= 0x0005) && (address < 0x000A))
  {
    status = bus_interface_write(&apu->ch2.bus_interface, address, data);
  }
  else if ((address >= 0x000A) && (address < 0x000F))
  {
    status = bus_interface_write(&apu->ch3.bus_interface, address, data);
  }
  else if ((address >= 0x000F) && (address < 0x0014))
  {
    status = bus_interface_write(&apu->ch4.bus_interface, address, data);
  }
  else if (address == 0x0014)
  {
    apu->registers.mvp = data;
  }
  else if (address == 0x0015)
  {
    apu->registers.sndp = data;
  }
  else if (address == 0x0016)
  {
    apu->registers.actl &= ~APU_ACTL_AUDIO_EN;
    apu->registers.actl |= (data & APU_ACTL_AUDIO_EN);
  }
  else if ((address >= 0x0020) && (address < 0x0030))
  {
    status = bus_interface_write(&apu->ch3.wave_ram.bus_interface, address, data);
  }
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}
