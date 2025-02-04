#include "unity.h"

#include "rtc.h"

#include <string.h>

#include "mock_time.h"

TEST_FILE("rtc.c")

static rtc_handle_t rtc;
static time_t timestamp;
static const time_t initial_timestamp = 10000;

void setUp(void)
{
  memset(&rtc, 0, sizeof(rtc_handle_t));
  timestamp = initial_timestamp;
}

void tearDown(void)
{
}

void test_rtc_init(void)
{
  time_ExpectAndReturn(NULL, timestamp);

  TEST_ASSERT_EQUAL_INT(STATUS_OK, rtc_init(&rtc));
}

void test_rtc__invalid_args(void)
{
  uint8_t data;
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_init(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_sync(NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_latch(NULL, true));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_read(NULL, &data));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_read(&rtc, NULL));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_write(NULL, 0xFF));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_NULL_PTR, rtc_select_reg(NULL, RTC_REG_SECONDS));
  TEST_ASSERT_EQUAL_INT(STATUS_ERR_INVALID_ARG, rtc_select_reg(&rtc, 8));
}
