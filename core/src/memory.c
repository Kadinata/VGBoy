#include "memory.h"

#include <stdint.h>

#include "status_code.h"

status_code_t mem_read_8(uint16_t const addr, uint8_t *const data) { return STATUS_OK; }
status_code_t mem_read_16(uint16_t const addr, uint16_t *const data) { return STATUS_OK; }
status_code_t mem_write_8(uint16_t const addr, uint8_t const data) { return STATUS_OK; }
status_code_t mem_write_16(uint16_t const addr, uint16_t const data) { return STATUS_OK; }