#include "unity.h"

#include <string.h>
#include <stdio.h>

#include "rtc.h"
#include "time_helper.h"
#include "rtc_test_helper.h"
#include "mock_time.h"

static rtc_handle_t rtc;
static time_t const starting_timer_value = 10000;

void setUp(void)
{
  memset(&rtc, 0, sizeof(rtc_handle_t));
  init_time(starting_timer_value);
  rtc_init_and_enable(&rtc);
}

void tearDown(void)
{
}

void test_when_not_halted_the_rtc_is_counting(void)
{
  TEST_ASSERT_EQUAL_INT(0, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(starting_timer_value, rtc.state.prev_timestamp);

  delay_seconds(100);

  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_sync(&rtc));

  TEST_ASSERT_EQUAL_INT(100, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(get_time(), rtc.state.prev_timestamp);
}

void test_when_halted_counting_is_paused(void)
{
  TEST_ASSERT_EQUAL_INT(0, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(starting_timer_value, rtc.state.prev_timestamp);

  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl | RTC_HALT));

  delay_seconds(100);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_sync(&rtc));

  TEST_ASSERT_EQUAL_INT(0, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(starting_timer_value, rtc.state.prev_timestamp);
}

void test_when_unhalted_counting_is_resumed(void)
{
  TEST_ASSERT_EQUAL_INT(0, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(starting_timer_value, rtc.state.prev_timestamp);

  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl | RTC_HALT));

  delay_seconds(100);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_sync(&rtc));

  TEST_ASSERT_EQUAL_INT(0, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(starting_timer_value, rtc.state.prev_timestamp);

  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl & ~RTC_HALT));

  delay_seconds(100);

  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_sync(&rtc));

  TEST_ASSERT_EQUAL_INT(100, rtc.state.current_timestamp);
  TEST_ASSERT_EQUAL_INT(get_time(), rtc.state.prev_timestamp);
}

void test_when_counter_overflows_the_corresponding_flag_is_set(void)
{
  /* Delay to the max counter value */
  delay(0x1FF, 23, 59, 59);

  /* Latch counter value to registers */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 0));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 1));

  /** Verify register values are updated and no overflow */
  TEST_ASSERT_EQUAL_INT(59, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(59, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(23, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0xFF, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);
  TEST_ASSERT_FALSE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));

  /* Delay 1 second */
  delay_seconds(1);

  /* The flag should be set even without latching */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_sync(&rtc));

  TEST_ASSERT_EQUAL_INT(59, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(59, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(23, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0xFF, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);
  TEST_ASSERT_TRUE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));

  /* Latch counter value to registers again */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 0));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 1));

  /** Verify register values are updated and there is an overflow */
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);
  TEST_ASSERT_TRUE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));
}