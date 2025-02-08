#ifndef __RTC_TEST_HELPER_H__
#define __RTC_TEST_HELPER_H__

#include "unity.h"
#include "rtc.h"

#include "mock_time.h"

void rtc_init_and_enable(rtc_handle_t *const rtc);

#endif
