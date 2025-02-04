#include "unity.h"

#include <string.h>

#include "rtc.h"
#include "time_helper.h"
#include "mock_time.h"

static rtc_handle_t rtc;

void setUp(void)
{
  memset(&rtc, 0, sizeof(rtc_handle_t));
  init_time(10000);
}

void tearDown(void)
{
}

void test_when_writing_0_then_1_rtc_registers_do_latch(void)
{
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_init(&rtc));

  delay(1, 2, 3, 4);

  /** Verify initial register values */
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);

  /** Set the latch to false */
  rtc.state.prev_latch = true;
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, false));
  TEST_ASSERT_FALSE(rtc.state.prev_latch);

  /** Verify register values are unchanged */
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);

  /** Set the latch to true */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, true));
  TEST_ASSERT_TRUE(rtc.state.prev_latch);

  /** Verify register values are updated */
  TEST_ASSERT_EQUAL_INT(4, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(3, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(2, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);
}

void test_when_writing_1_then_1_rtc_registers_do_not_latch(void)
{
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_init(&rtc));

  delay(1, 2, 3, 4);

  /** Verify initial register values */
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);

  /** Set the latch to false */
  rtc.state.prev_latch = true;
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, true));
  TEST_ASSERT_TRUE(rtc.state.prev_latch);

  /** Verify register values are unchanged */
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);
}
