#ifndef __TEST_TIME_HELPER_H__
#define __TEST_TIME_HELPER_H__

#include <stdint.h>
#include "time.h"

void init_time(time_t start_value);
void delay_seconds(time_t seconds);
void delay(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
time_t get_time(void);

#endif
