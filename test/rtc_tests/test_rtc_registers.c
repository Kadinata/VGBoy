#include "unity.h"

#include <string.h>

#include "rtc.h"
#include "time_helper.h"
#include "rtc_test_helper.h"
#include "mock_time.h"

static rtc_handle_t rtc;

void stub_latch_rtc_registers(bool update_time)
{
  if (update_time)
  {
    time_ExpectAndReturn(NULL, get_time());
  }
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 0));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_latch(&rtc, 1));
}

void setUp(void)
{
  memset(&rtc, 0, sizeof(rtc_handle_t));
  init_time(10000);
  rtc_init_and_enable(&rtc);
}

void tearDown(void)
{
}

/* Test register select */
void test_register_select_to_read(void)
{
  uint8_t data;

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /** Latch the counter values */
  stub_latch_rtc_registers(true);

  /** Read register values */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_INT(5, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_MINUTES));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_INT(4, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_HOURS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_INT(3, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAYS_L));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_INT(2, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_INT(1, data & RTC_DAY_MSB);
  TEST_ASSERT_EQUAL_INT(0, data & RTC_HALT);
  TEST_ASSERT_EQUAL_INT(0, data & RTC_DAY_OVERFLOW);
}

void test_when_halted_register_writes_do_update_time_values(void)
{
  uint8_t data;

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /* Halt the timer */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, RTC_HALT));

  /* Latch register values */
  stub_latch_rtc_registers(false);

  /* Update each register and verify its value is not updated before latching */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 40));
  TEST_ASSERT_EQUAL_INT(5, rtc.registers.seconds);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_MINUTES));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 30));
  TEST_ASSERT_EQUAL_INT(4, rtc.registers.minutes);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_HOURS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 20));
  TEST_ASSERT_EQUAL_INT(3, rtc.registers.hours);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAYS_L));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 10));
  TEST_ASSERT_EQUAL_INT(2, rtc.registers.days);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl & ~RTC_DAY_MSB));
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);

  /* Latch the counter values to the registers */
  stub_latch_rtc_registers(false);

  /** Verify register values are now updated */
  TEST_ASSERT_EQUAL_INT(40, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(30, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(20, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(10, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(0, rtc.registers.dctrl & RTC_DAY_MSB);
}

void test_when_not_halted_register_writes_do_not_update_time_values(void)
{
  uint8_t data;

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /* Latch register values */
  stub_latch_rtc_registers(true);

  /* Update each register and verify its value is not updated before latching */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 40));
  TEST_ASSERT_EQUAL_INT(5, rtc.registers.seconds);

  TEST_ASSERT_EQUAL_INT(4, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_MINUTES));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 30));
  TEST_ASSERT_EQUAL_INT(4, rtc.registers.minutes);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_HOURS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 20));
  TEST_ASSERT_EQUAL_INT(3, rtc.registers.hours);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAYS_L));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 10));
  TEST_ASSERT_EQUAL_INT(2, rtc.registers.days);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl & ~RTC_DAY_MSB));
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);

  /* Latch the counter values to the registers */
  stub_latch_rtc_registers(true);

  /** Verify register values remain unchanged */
  TEST_ASSERT_EQUAL_INT(5, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(4, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(3, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(2, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);
}

void test_writes_to_control_and_status_bits_unaffected_by_halt(void)
{
  uint8_t data;

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /* Write halt */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl | RTC_HALT));
  TEST_ASSERT_TRUE(!!(rtc.registers.dctrl & RTC_HALT));

  /* Write to overflow */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl | RTC_DAY_OVERFLOW));
  TEST_ASSERT_TRUE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));

  /* Write to halt again */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl & ~RTC_HALT));
  TEST_ASSERT_FALSE(!!(rtc.registers.dctrl & RTC_HALT));

  /* Write to overflow again */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, rtc.registers.dctrl & ~RTC_DAY_OVERFLOW));
  TEST_ASSERT_FALSE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));
}

void test_register_reads_are_ignored_when_rtc_is_disabled(void)
{
  uint8_t data;

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /** Latch the counter values */
  stub_latch_rtc_registers(true);

  /* Disable the RTC */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_enable(&rtc, false));

  /** Read register values */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_MINUTES));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_HOURS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAYS_L));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, data);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_read(&rtc, &data));
  TEST_ASSERT_EQUAL_HEX8(0xFF, data);
}

void test_register_writes_are_ignored_when_rtc_is_disabled(void)
{
  uint8_t data;

  /* Advance the counter */
  delay(0x102, 3, 4, 5);

  /* Halt the timer */
  time_ExpectAndReturn(NULL, get_time());
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, RTC_HALT));

  /* Latch register values */
  stub_latch_rtc_registers(false);

  /* Disable the RTC */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_enable(&rtc, false));

  /* Update each register and verify its value is not updated before latching */
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 40));

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_MINUTES));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 30));

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_HOURS));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 20));

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAYS_L));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, 10));

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_select_reg(&rtc, RTC_REG_DAY_CTRL));
  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_write(&rtc, ~rtc.registers.dctrl));

  /* Latch the counter values to the registers */
  stub_latch_rtc_registers(false);

  /** Verify register values are now updated */
  TEST_ASSERT_EQUAL_INT(5, rtc.registers.seconds);
  TEST_ASSERT_EQUAL_INT(4, rtc.registers.minutes);
  TEST_ASSERT_EQUAL_INT(3, rtc.registers.hours);
  TEST_ASSERT_EQUAL_INT(2, rtc.registers.days);
  TEST_ASSERT_EQUAL_INT(1, rtc.registers.dctrl & RTC_DAY_MSB);
  TEST_ASSERT_TRUE(!!(rtc.registers.dctrl & RTC_HALT));
  TEST_ASSERT_FALSE(!!(rtc.registers.dctrl & RTC_DAY_OVERFLOW));
}