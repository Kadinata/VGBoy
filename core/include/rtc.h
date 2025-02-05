#ifndef __DMG_RTC_H__
#define __DMG_RTC_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "status_code.h"

typedef enum
{
  RTC_DAY_MSB = (1 << 0),
  RTC_HALT = (1 << 6),
  RTC_DAY_OVERFLOW = (1 << 7),
} rtc_dctrl_mask_t;

typedef enum
{
  RTC_REG_SECONDS = 0,
  RTC_REG_MINUTES,
  RTC_REG_HOURS,
  RTC_REG_DAYS_L,
  RTC_REG_DAY_CTRL,
  RTC_REG_MAX,
} rtc_reg_type_t;

typedef union
{
  struct
  {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days;
    uint8_t dctrl;
  };
  uint8_t buffer[RTC_REG_MAX];
} rtc_registers_t;

typedef struct
{
  time_t current_timestamp;
  time_t prev_timestamp;
  rtc_reg_type_t active_reg;
  uint8_t prev_latch;
  bool present;
  bool enabled;
  bool mapped_to_memory;
} rtc_state_t;

typedef struct
{
  rtc_registers_t registers;
  rtc_state_t state;
} rtc_handle_t;

status_code_t rtc_init(rtc_handle_t *const rtc);
status_code_t rtc_sync(rtc_handle_t *const rtc);
status_code_t rtc_latch(rtc_handle_t *const rtc, uint8_t const command);
status_code_t rtc_read(rtc_handle_t *const rtc, uint8_t *const data);
status_code_t rtc_write(rtc_handle_t *const rtc, uint8_t const data);
status_code_t rtc_select_reg(rtc_handle_t *const rtc, rtc_reg_type_t const reg);

#endif /* __DMG_RTC_H__ */
