#ifndef __DMG_EMULATOR_H__
#define __DMG_EMULATOR_H__

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "ram.h"
#include "oam.h"
#include "io.h"
#include "timer.h"
#include "timing_sync.h"
#include "status_code.h"

typedef struct
{
  cpu_state_t cpu_state;
  data_bus_handle_t bus_handle;
  rom_handle_t rom;
  ram_handle_t ram;
  oam_handle_t oam;
  io_handle_t io;
  timer_handle_t tmr;
  interrupt_handle_t interrupt;
  timing_sync_handle_t sync_handle;
} emulator_t;

status_code_t emulator_init(emulator_t *const emulator);
status_code_t emulator_cleanup(emulator_t *const emulator);

#endif /* __DMG_EMULATOR_H__ */
