#ifndef __DMG_EMULATOR_H__
#define __DMG_EMULATOR_H__

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "status_code.h"

typedef struct
{
  cpu_state_t cpu_state;
  rom_handle_t rom;
  data_bus_handle_t bus_handle;
} emulator_t;

status_code_t emulator_init(emulator_t *const emulator);
status_code_t emulator_cleanup(emulator_t *const emulator);

#endif /* __DMG_EMULATOR_H__ */
