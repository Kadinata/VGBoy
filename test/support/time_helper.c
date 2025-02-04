#include "time_helper.h"

#include <stdint.h>
#include "time.h"

static time_t timer;

void init_time(time_t start_value)
{
  timer = start_value;
}

void delay_seconds(time_t seconds)
{
  timer += seconds;
}

void inline delay(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  delay_seconds((days * 60 * 60 * 24) + (hours * 60 * 60) + (minutes * 60) + seconds);
}

time_t get_time(void)
{
  return timer;
}
