#include "debug_serial.h"

#include <stdint.h>
#include "logging.h"

static uint8_t serial_data[2];
static char serial_buf[1024] = {0};
static uint16_t buf_ptr = 0;

void serial_write(uint8_t index, uint8_t data)
{
  if (index > 1)
  {
    return;
  }
  serial_data[index] = data;
}

void serial_read(uint8_t index, uint8_t *data)
{
  if (index > 1)
  {
    return;
  }
  *data = serial_data[index];
}

void serial_check(void)
{
  if (serial_data[1] == 0x81)
  {
    serial_buf[buf_ptr++] = (char)serial_data[0];
    buf_ptr %= 1024;
    serial_data[1] = 0;
    fprintf(stderr, "%c", (char)serial_data[0]);
  }

  // if (serial_buf[0])
  // {
  //   Log_D("S OUT: %s", serial_buf);
  // }
}
