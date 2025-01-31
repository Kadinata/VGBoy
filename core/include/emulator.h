#ifndef __DMG_EMULATOR_H__
#define __DMG_EMULATOR_H__

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "data_bus.h"
#include "ram.h"
#include "io.h"
#include "dma.h"
#include "mbc.h"
#include "joypad.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "status_code.h"
#include "callback.h"

typedef enum {
  EMU_MODE_RUNNING,
  EMU_MODE_STOPPED,
} emulator_state_t;

typedef struct
{
  cpu_state_t cpu_state;
  data_bus_handle_t bus_handle;
  ram_handle_t ram;
  io_handle_t io;
  timer_handle_t tmr;
  dma_handle_t dma;
  ppu_handle_t ppu;
  apu_handle_t apu;
  mbc_handle_t mbc;
  joypad_handle_t joypad;
  callback_t cycle_sync_callback;
  emulator_state_t state;
} emulator_t;

status_code_t emulator_init(emulator_t *const emulator);
status_code_t emulator_load_cartridge(emulator_t *const emulator, const char *file);
status_code_t emulator_run(emulator_t *const emulator);
void emulator_stop(emulator_t *const emulator);
status_code_t emulator_cleanup(emulator_t *const emulator);

#endif /* __DMG_EMULATOR_H__ */
