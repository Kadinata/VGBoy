#ifndef __DBG_SERIAL_H__
#define __DBG_SERIAL_H__

#include <stdint.h>

void serial_write(uint8_t index, uint8_t data);
void serial_read(uint8_t index, uint8_t *data);
void serial_check(void);

#endif /* __DBG_SERIAL_H__ */
