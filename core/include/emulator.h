#ifndef __DMG_EMULATOR_H__
#define __DMG_EMULATOR_H__

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "ram.h"
#include "io.h"
#include "dma.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"
#include "status_code.h"
#include "callback.h"

typedef struct
{
  cpu_state_t cpu_state;
  data_bus_handle_t bus_handle;
  rom_handle_t rom;
  ram_handle_t ram;
  io_handle_t io;
  timer_handle_t tmr;
  dma_handle_t dma;
  ppu_handle_t ppu;
  joypad_handle_t joypad;
  interrupt_handle_t interrupt;
  callback_t cycle_sync_callback;
} emulator_t;

status_code_t emulator_init(emulator_t *const emulator);
status_code_t emulator_cleanup(emulator_t *const emulator);

#endif /* __DMG_EMULATOR_H__ */
