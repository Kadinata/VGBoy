#ifndef __DMG_CPU_H__
#define __DMG_CPU_H__

#include <stdint.h>

#include "status_code.h"
#include "sys_def.h"

status_code_t cpu_emulation_cycle(cpu_state_t *const state);

#endif /* __DMG_CPU_H__ */
