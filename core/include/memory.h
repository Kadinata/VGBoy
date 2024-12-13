#ifndef __DMG_MEMORY_H__
#define __DMG_MEMORY_H__

#include <stdint.h>

#include "status_code.h"

status_code_t mem_read_8(uint16_t const addr, uint8_t *const data);
status_code_t mem_read_16(uint16_t const addr, uint16_t *const data);
status_code_t mem_write_8(uint16_t const addr, uint8_t const data);
status_code_t mem_write_16(uint16_t const addr, uint16_t const data);

#endif /* __DMG_MEMORY_H__ */
