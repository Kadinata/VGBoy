#include "apu_wave.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "apu_common.h"
#include "bus_interface.h"
#include "status_code.h"

static status_code_t apu_wave_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_wave_write(void *const resource, uint16_t const address, uint8_t const data);

static status_code_t apu_wave_ram_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_wave_ram_write(void *const resource, uint16_t const address, uint8_t const data);

static inline uint16_t get_channel_period(apu_wave_handle_t *const apu_wave);
static inline uint8_t get_current_amplitude(apu_wave_handle_t *const apu_wave);
static inline bool length_timer_enabled(apu_wave_handle_t *const apu_wave);
static inline void trigger_channel(apu_wave_handle_t *const apu_wave);

status_code_t apu_wave_init(apu_wave_handle_t *const apu_wave)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_wave);

  status_code_t status = STATUS_OK;

  status = bus_interface_init(&apu_wave->bus_interface, apu_wave_read, apu_wave_write, apu_wave);
  RETURN_STATUS_IF_NOT_OK(status);

  status = bus_interface_init(&apu_wave->wave_ram.bus_interface, apu_wave_ram_read, apu_wave_ram_write, &apu_wave->wave_ram);
  RETURN_STATUS_IF_NOT_OK(status);

  memset(apu_wave->wave_ram.data, 0, sizeof(apu_wave->wave_ram.data));

  return STATUS_OK;
}

status_code_t apu_wave_tick(apu_wave_handle_t *const apu_wave)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_wave);

  if (apu_wave->state.period_timer == 0)
  {
    apu_wave->state.period_timer = (2048 - get_channel_period(apu_wave)) >> 1;
    apu_wave->state.sample_index++;
    apu_wave->state.sample_index &= 0x1F;
  }

  apu_wave->state.period_timer--;
  return STATUS_OK;
}

status_code_t apu_wave_reset(apu_wave_handle_t *const apu_wave)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_wave);

  memset(&apu_wave->state, 0, sizeof(apu_wave_state_t));
  memset(&apu_wave->registers, 0, sizeof(apu_wave_registers_t));

  return STATUS_OK;
}

status_code_t apu_wave_sample(apu_wave_handle_t *const apu_wave, float *const sample_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_wave);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(sample_out);

  static uint8_t const volume_shift_table[] = {4, 0, 1, 2};

  uint8_t vol_shift_index = (apu_wave->registers.vol & APU_WAVE_VOL) >> 5;
  uint8_t volume_shift = volume_shift_table[vol_shift_index];

  *sample_out = apu_wave->state.enabled ? ((get_current_amplitude(apu_wave) >> volume_shift) / 15.0f) : 0;
  return STATUS_OK;
}

status_code_t apu_wave_handle_frame_sequencer(apu_wave_handle_t *const apu_wave, uint8_t const frame_step)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_wave);

  if (((frame_step & 0x1) == 0) && length_timer_enabled(apu_wave) && apu_wave->state.length_timer)
  {
    apu_wave->state.length_timer--;
    apu_wave->state.enabled = (apu_wave->state.length_timer > 0);
  }

  return STATUS_OK;
}

static inline uint8_t get_current_amplitude(apu_wave_handle_t *const apu_wave)
{
  uint8_t sample_index = apu_wave->state.sample_index;
  return (sample_index & 0x1) ? (apu_wave->wave_ram.data[sample_index >> 1] & 0xF) : (apu_wave->wave_ram.data[sample_index >> 1] >> 4);
}

static inline uint16_t get_channel_period(apu_wave_handle_t *const apu_wave)
{
  return ((apu_wave->registers.phctl & APU_WAVE_PERIOD_MSB) << 8) | apu_wave->registers.plow;
}

static inline bool length_timer_enabled(apu_wave_handle_t *const apu_wave)
{
  return apu_wave->registers.phctl & APU_LENGTH_EN;
}

static inline void trigger_channel(apu_wave_handle_t *const apu_wave)
{
  apu_wave->state.enabled = !!(apu_wave->registers.dacen & APU_WAVE_DAC_EN);
  apu_wave->state.length_timer = apu_wave->state.length_timer ? apu_wave->state.length_timer : 256;
  apu_wave->state.sample_index = 0;
}

static status_code_t apu_wave_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  static const uint8_t read_masks[] = {0x7F, 0xFF, 0x9F, 0xFF, 0xBF};

  apu_wave_handle_t *const apu_wave = (apu_wave_handle_t *)resource;

  switch (address)
  {
  case 0x0000:
    *data = apu_wave->registers.dacen;
    break;
  case 0x0001:
    *data = 0xFF; /* NR31 is write only */
    break;
  case 0x0002:
    *data = apu_wave->registers.vol;
    break;
  case 0x0003:
    *data = 0xFF; /* NR33 is write only */
    break;
  case 0x0004:
    *data = apu_wave->registers.phctl;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  *data |= read_masks[address];

  return STATUS_OK;
}

static status_code_t apu_wave_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  apu_wave_handle_t *const apu_wave = (apu_wave_handle_t *)resource;

  switch (address)
  {
  case 0x0000:
    apu_wave->registers.dacen = (data & APU_WAVE_DAC_EN);
    apu_wave->state.enabled = (apu_wave->state.enabled && !!(data & APU_WAVE_DAC_EN));
    break;
  case 0x0001:
    apu_wave->registers.ltmr = data;
    apu_wave->state.length_timer = (256 - data);
    break;
  case 0x0002:
    apu_wave->registers.vol = data & APU_WAVE_VOL;
    break;
  case 0x0003:
    apu_wave->registers.plow = data;
    break;
  case 0x0004:
    apu_wave->registers.phctl = data;
    if (data & APU_TRIGGER_EN)
    {
      trigger_channel(apu_wave);
    }
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}

static status_code_t apu_wave_ram_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(((apu_wave_ram_t *)resource)->data), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  *data = ((apu_wave_ram_t *)resource)->data[address];

  return STATUS_OK;
}

static status_code_t apu_wave_ram_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(address >= sizeof(((apu_wave_ram_t *)resource)->data), STATUS_ERR_ADDRESS_OUT_OF_BOUND);

  ((apu_wave_ram_t *)resource)->data[address] = data;

  return STATUS_OK;
}
