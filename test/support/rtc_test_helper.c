#include "rtc_test_helper.h"

#include "unity.h"
#include "rtc.h"
#include "time_helper.h"
#include "mock_time.h"

void rtc_init_and_enable(rtc_handle_t *const rtc)
{
  time_ExpectAndReturn(NULL, get_time());
  rtc_init(rtc);
  rtc_enable(rtc, true);
}