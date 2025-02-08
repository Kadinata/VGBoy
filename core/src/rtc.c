#include "rtc.h"

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "status_code.h"

#define MAX_RTC_DAYS (0x1FF)
#define MAX_RTC_VALUE ((MAX_RTC_DAYS + 1) * 60 * 60 * 24)

static void rtc_halt(rtc_handle_t *const rtc, bool halt);
static void rtc_sync_to_regs(rtc_handle_t *const rtc, rtc_registers_t *const regs);
static inline void rtc_check_overflow(rtc_handle_t *const rtc);
static inline void load_timestamp_to_regs(rtc_handle_t *const rtc, rtc_registers_t *const regs);
static inline bool is_halted(rtc_handle_t *const rtc);

static const uint8_t rtc_register_masks[] = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};

status_code_t rtc_init(rtc_handle_t *const rtc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);

  rtc->state.present = true;
  rtc->state.enabled = false;
  rtc->state.mapped_to_memory = false;
  rtc->state.current_timestamp = 0;
  rtc->state.prev_timestamp = time(NULL);

  return STATUS_OK;
}

status_code_t rtc_select_reg(rtc_handle_t *const rtc, rtc_reg_type_t const reg)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(reg >= sizeof(rtc_registers_t), STATUS_ERR_INVALID_ARG);

  rtc->state.active_reg = reg;

  return STATUS_OK;
}

status_code_t rtc_enable(rtc_handle_t *const rtc, bool enable)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);

  rtc->state.enabled = rtc->state.present && enable;

  return STATUS_OK;
}

status_code_t rtc_latch(rtc_handle_t *const rtc, uint8_t const command)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);

  if ((rtc->state.prev_latch == 0) && (command == 1))
  {
    rtc_sync(rtc);
    load_timestamp_to_regs(rtc, &rtc->registers);
  }

  rtc->state.prev_latch = command;

  return STATUS_OK;
}

status_code_t rtc_read(rtc_handle_t *const rtc, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  *data = rtc->state.enabled ? (rtc->registers.buffer[rtc->state.active_reg] & rtc_register_masks[rtc->state.active_reg]) : 0xFF;

  return STATUS_OK;
}

status_code_t rtc_write(rtc_handle_t *const rtc, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(!rtc->state.enabled, STATUS_OK);

  rtc_registers_t mirror_regs = {0};

  if (is_halted(rtc))
  {
    load_timestamp_to_regs(rtc, &mirror_regs);
    mirror_regs.buffer[rtc->state.active_reg] = data & rtc_register_masks[rtc->state.active_reg];
    rtc_sync_to_regs(rtc, &mirror_regs);
  }

  if (rtc->state.active_reg == RTC_REG_DAY_CTRL)
  {
    rtc_halt(rtc, !!(data & RTC_HALT));
    rtc->registers.dctrl &= ~(RTC_DAY_OVERFLOW | RTC_HALT);
    rtc->registers.dctrl |= (data & (RTC_DAY_OVERFLOW | RTC_HALT));
  }

  return STATUS_OK;
}

status_code_t rtc_sync(rtc_handle_t *const rtc)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(rtc);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(is_halted(rtc), STATUS_OK);

  time_t delta = time(NULL) - rtc->state.prev_timestamp;
  rtc->state.current_timestamp += delta;
  rtc->state.prev_timestamp += delta;

  rtc_check_overflow(rtc);

  return STATUS_OK;
}

bool rtc_is_present(rtc_handle_t *const rtc)
{
  return rtc && rtc->state.present;
}

static void rtc_halt(rtc_handle_t *const rtc, bool halt)
{
  if (is_halted(rtc) && !halt)
  {
    rtc->state.prev_timestamp = time(NULL);
  }
  else if (!is_halted(rtc) && halt)
  {
    rtc_sync(rtc);
  }
}

static void rtc_sync_to_regs(rtc_handle_t *const rtc, rtc_registers_t *const regs)
{
  uint16_t total_days = ((regs->dctrl & RTC_DAY_MSB) << 8) | regs->days;
  rtc->state.current_timestamp = 0;
  rtc->state.current_timestamp += regs->seconds;
  rtc->state.current_timestamp += regs->minutes * 60;
  rtc->state.current_timestamp += regs->hours * 60 * 60;
  rtc->state.current_timestamp += total_days * 60 * 60 * 24;

  rtc_check_overflow(rtc);
}

static inline void rtc_check_overflow(rtc_handle_t *const rtc)
{
  if ((rtc->state.current_timestamp / 60 / 60 / 24) > MAX_RTC_DAYS)
  {
    rtc->state.current_timestamp %= MAX_RTC_VALUE;
    rtc->registers.dctrl |= RTC_DAY_OVERFLOW;
  }
}

static inline void load_timestamp_to_regs(rtc_handle_t *const rtc, rtc_registers_t *const regs)
{
  regs->seconds = rtc->state.current_timestamp % 60;
  regs->minutes = (rtc->state.current_timestamp / 60) % 60;
  regs->hours = (rtc->state.current_timestamp / 60 / 60) % 24;
  regs->days = (rtc->state.current_timestamp / 60 / 60 / 24) & 0xFF;
  regs->dctrl &= ~RTC_DAY_MSB;
  regs->dctrl |= ((rtc->state.current_timestamp / 60 / 60 / 24) & 0x100) ? RTC_DAY_MSB : 0;
}

static inline bool is_halted(rtc_handle_t *const rtc)
{
  return !!(rtc->registers.dctrl & RTC_HALT);
}
