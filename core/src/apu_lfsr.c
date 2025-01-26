#include "apu_lfsr.h"

#include <stdint.h>
#include <stdbool.h>

#include "apu_common.h"
#include "bus_interface.h"
#include "status_code.h"

static status_code_t apu_lfsr_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t apu_lfsr_write(void *const resource, uint16_t const address, uint8_t const data);

static status_code_t update_envelope(apu_lfsr_handle_t *const apu_lfsr);
static inline uint8_t get_current_amplitude(apu_lfsr_handle_t *const apu_lfsr);
static inline void trigger_channel(apu_lfsr_handle_t *const apu_lfsr);
static inline bool length_timer_enabled(apu_lfsr_handle_t *const apu_lfsr);

status_code_t apu_lfsr_init(apu_lfsr_handle_t *const apu_lfsr)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_lfsr);

  status_code_t status = STATUS_OK;

  status = bus_interface_init(&apu_lfsr->bus_interface, apu_lfsr_read, apu_lfsr_write, apu_lfsr);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t apu_lfsr_tick(apu_lfsr_handle_t *const apu_lfsr)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_lfsr);

  if (apu_lfsr->state.period_counter == 0)
  {
    uint8_t clock_divider = apu_lfsr->registers.frqrand & APU_FRQRAND_CLK_DIV;
    uint8_t clock_shift = (apu_lfsr->registers.frqrand & APU_FRQRAND_CLK_SHIFT) >> 4;

    apu_lfsr->state.period_counter = (clock_divider ? (clock_divider << 4) : 8) << clock_shift;

    uint8_t xor_result = (apu_lfsr->state.lfsr & 0x1) ^ ((apu_lfsr->state.lfsr & 0x2) >> 1);
    apu_lfsr->state.lfsr = (apu_lfsr->state.lfsr >> 1) | (xor_result << 14);

    if (apu_lfsr->registers.frqrand & APU_FRQRAND_LFSR_WIDTH)
    {
      apu_lfsr->state.lfsr &= ~(1 << 6);
      apu_lfsr->state.lfsr |= (xor_result << 6);
    }
  }

  apu_lfsr->state.period_counter--;
  return STATUS_OK;
}

status_code_t apu_lfsr_sample(apu_lfsr_handle_t *const apu_lfsr, float *const sample_out)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_lfsr);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(sample_out);

  *sample_out = apu_lfsr->state.enabled ? (get_current_amplitude(apu_lfsr) * (apu_lfsr->state.volume / 15.0f)) : 0.0f;
  return STATUS_OK;
}

status_code_t apu_lfsr_handle_frame_sequencer(apu_lfsr_handle_t *const apu_lfsr, uint8_t const frame_step)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(apu_lfsr);

  if (((frame_step & 0x1) == 0) && length_timer_enabled(apu_lfsr) && apu_lfsr->state.length_timer)
  {
    apu_lfsr->state.length_timer--;
    apu_lfsr->state.enabled = (apu_lfsr->state.length_timer > 0);
  }

  if (frame_step == 0x7)
  {
    return update_envelope(apu_lfsr);
  }

  return STATUS_OK;
}

static status_code_t update_envelope(apu_lfsr_handle_t *const apu_lfsr)
{
  const uint8_t sweep_period = apu_lfsr->registers.volenv & APU_ENVELOPE_PACE;
  const uint8_t envelope_dir = apu_lfsr->registers.volenv & APU_ENVELOPE_DIR;

  if ((sweep_period == 0) || (apu_lfsr->state.envelope_timer == 0))
  {
    return STATUS_OK;
  }

  apu_lfsr->state.envelope_timer--;

  if (apu_lfsr->state.envelope_timer == 0)
  {
    apu_lfsr->state.envelope_timer = sweep_period;

    if (envelope_dir && (apu_lfsr->state.volume < 0xF))
    {
      apu_lfsr->state.volume++;
    }
    else if (!envelope_dir && (apu_lfsr->state.volume > 0x0))
    {
      apu_lfsr->state.volume--;
    }
  }

  return STATUS_OK;
}

static inline uint8_t get_current_amplitude(apu_lfsr_handle_t *const apu_lfsr)
{
  return ~apu_lfsr->state.lfsr & 0x1;
}

static inline bool length_timer_enabled(apu_lfsr_handle_t *const apu_lfsr)
{
  return !!(apu_lfsr->registers.ctrl & APU_LENGTH_EN);
}

static inline void trigger_channel(apu_lfsr_handle_t *const apu_lfsr)
{
  apu_lfsr->state.enabled = !!(apu_lfsr->registers.volenv & APU_CHANNEL_ENABLED);
  apu_lfsr->state.volume = (apu_lfsr->registers.volenv & APU_START_VOLUME) >> 4;
  apu_lfsr->state.length_timer = apu_lfsr->state.length_timer ? apu_lfsr->state.length_timer : 64;
  apu_lfsr->state.envelope_timer = apu_lfsr->registers.volenv & APU_ENVELOPE_PACE;
  apu_lfsr->state.lfsr = 0x7FFF;
}

static status_code_t apu_lfsr_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  static const uint8_t read_masks[] = {0xFF, 0xFF, 0x00, 0x00, 0xBF};

  apu_lfsr_handle_t *const apu_lfsr = (apu_lfsr_handle_t *)resource;

  switch (address)
  {
  case 0x0000: /* No NR40 */
    *data = 0xFF;
    break;
  case 0x0001: /* NR41 */
    *data = apu_lfsr->registers.ltmr;
    break;
  case 0x0002: /* NR42 */
    *data = apu_lfsr->registers.volenv;
    break;
  case 0x0003: /* NR43 */
    *data = apu_lfsr->registers.frqrand;
    break;
  case 0x0004: /* NR44 */
    *data = apu_lfsr->registers.ctrl;
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  *data |= read_masks[address];

  return STATUS_OK;
}

static status_code_t apu_lfsr_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  apu_lfsr_handle_t *const apu_lfsr = (apu_lfsr_handle_t *)resource;

  switch (address)
  {
  case 0x0000: /* No NR40 */
    break;
  case 0x0001: /* NR41 */
    apu_lfsr->registers.ltmr = data;
    apu_lfsr->state.length_timer = 64 - (data & APU_LENGTH_TIMER);
    break;
  case 0x0002: /* NR42 */
    apu_lfsr->registers.volenv = data;
    apu_lfsr->state.enabled = !!(data & APU_CHANNEL_ENABLED);
    break;
  case 0x0003: /* NR43 */
    apu_lfsr->registers.frqrand = data;
    break;
  case 0x0004: /* NR44 */
    apu_lfsr->registers.ctrl = data;
    if (data & APU_TRIGGER_EN)
    {
      trigger_channel(apu_lfsr);
    }
    break;
  default:
    return STATUS_ERR_ADDRESS_OUT_OF_BOUND;
  }

  return STATUS_OK;
}
