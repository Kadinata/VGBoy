#ifndef __CPU_TEST_HELPER_H__
#define __CPU_TEST_HELPER_H__

#include <stdint.h>

#include "cpu.h"

#define TEST_PC_INIT_VALUE (0x100)

void stub_cpu_state_init(cpu_state_t *state);
void stub_mem_read_8(uint16_t addr, uint8_t *data);
void stub_mem_read_16(uint16_t addr, uint16_t *data);
void stub_mem_write_16(uint16_t addr, uint16_t data);

#endif /* __CPU_TEST_HELPER_H__ */