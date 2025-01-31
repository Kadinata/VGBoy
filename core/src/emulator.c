#include "emulator.h"

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "ram.h"
#include "oam.h"
#include "io.h"
#include "dma.h"
#include "mbc.h"
#include "joypad.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "logging.h"
#include "bus_interface.h"
#include "status_code.h"
#include "callback.h"
#include "logging.h"

static status_code_t sync_callback_handler(void *const ctx, const void *arg);
static inline status_code_t module_init(emulator_t *const emulator);
static inline status_code_t configure_data_bus(emulator_t *const emulator);

status_code_t emulator_init(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  status_code_t status = STATUS_OK;

  emulator->ram.wram.offset = 0xC000;
  emulator->ram.vram.offset = 0x8000;
  emulator->ram.hram.offset = 0xFF80;
  emulator->ram.bus_interface.offset = 0x0000;
  emulator->ppu.oam.bus_interface.offset = 0xFE00;
  emulator->io.bus_interface.offset = 0xFF00;
  emulator->cpu_state.interrupt.bus_interface.offset = 0xFF00;
  emulator->state = EMU_MODE_RUNNING;
  emulator->prev_frame_count = emulator->ppu.current_frame;

  status = module_init(emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  status = configure_data_bus(emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t emulator_run(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  status_code_t status = STATUS_OK;

  while (emulator->state == EMU_MODE_RUNNING)
  {
    status = emulator_run_frame(emulator);
    if (status != STATUS_OK)
    {
      Log_E("CPU emulation cycle encountered an error: %d", status);
      break;
    }
    else if (emulator->cpu_state.run_mode == RUN_MODE_STOPPED)
    {
      Log_I("CPU Stopped!");
      break;
    }
  }

  return status;
}

void emulator_stop(emulator_t *const emulator)
{
  if (emulator)
  {
    emulator->state = EMU_MODE_STOPPED;
  }
}

status_code_t emulator_cleanup(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  return mbc_cleanup(&emulator->mbc);
}

static status_code_t sync_callback_handler(void *const ctx, const void *arg)
{
  emulator_t *const emulator = (emulator_t *)ctx;
  uint8_t const m_cycle_count = *(uint8_t *)arg;
  status_code_t status = STATUS_OK;

  for (uint8_t m = 0; m < m_cycle_count; m++)
  {
    for (uint8_t t = 0; t < 4; t++)
    {
      status = timer_tick(&emulator->tmr);
      RETURN_STATUS_IF_NOT_OK(status);

      status = ppu_tick(&emulator->ppu);
      RETURN_STATUS_IF_NOT_OK(status);
    }

    status = dma_tick(&emulator->dma);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return status;
}

status_code_t emulator_run_frame(emulator_t *const emulator)
{
  status_code_t status = STATUS_OK;

  while(emulator->prev_frame_count == emulator->ppu.current_frame)
  {
    status = cpu_emulation_cycle(&emulator->cpu_state);
    RETURN_STATUS_IF_NOT_OK(status);

    if (emulator->cpu_state.run_mode == RUN_MODE_STOPPED)
    {
      break;
    }
  }

  emulator->prev_frame_count = emulator->ppu.current_frame;
  return STATUS_OK;
}

static inline status_code_t module_init(emulator_t *const emulator)
{
  status_code_t status = STATUS_OK;

  cpu_init_param_t cpu_init_params = {
      .bus_interface = &emulator->bus_handle.bus_interface,
      .cycle_sync_callback = &emulator->cycle_sync_callback,
  };

  io_init_param_t io_init_params = {
      .dma_handle = &emulator->dma,
      .int_bus_interface = &emulator->cpu_state.interrupt.bus_interface,
      .lcd_bus_interface = &emulator->ppu.lcd.bus_interface,
      .timer_bus_interface = &emulator->tmr.bus_interface,
      .joypad_bus_interface = &emulator->joypad.bus_interface,
      .apu_bus_interface = &emulator->apu.bus_interface,
  };

  ppu_init_param_t ppu_init_params = {
      .bus_interface = &emulator->bus_handle.bus_interface,
      .interrupt = &emulator->cpu_state.interrupt,
  };

  status = data_bus_init(&emulator->bus_handle);
  RETURN_STATUS_IF_NOT_OK(status);

  status = ram_init(&emulator->ram);
  RETURN_STATUS_IF_NOT_OK(status);

  status = mbc_init(&emulator->mbc);
  RETURN_STATUS_IF_NOT_OK(status);

  status = timer_init(&emulator->tmr, &emulator->cpu_state.interrupt);
  RETURN_STATUS_IF_NOT_OK(status);

  status = joypad_init(&emulator->joypad);
  RETURN_STATUS_IF_NOT_OK(status);

  status = io_init(&emulator->io, &io_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = dma_init(&emulator->dma, emulator->bus_handle.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = ppu_init(&emulator->ppu, &ppu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = apu_init(&emulator->apu);
  RETURN_STATUS_IF_NOT_OK(status);

  status = cpu_init(&emulator->cpu_state, &cpu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = callback_init(&emulator->cycle_sync_callback, sync_callback_handler, (void *)emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static inline status_code_t configure_data_bus(emulator_t *const emulator)
{
  status_code_t status = STATUS_OK;

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_BANK_0, emulator->mbc.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_SWITCHABLE_BANK, emulator->mbc.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_WRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_VRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_EXT_RAM, emulator->mbc.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_OAM, emulator->ppu.oam.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_HRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IO_REG, emulator->io.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IE_REG, emulator->cpu_state.interrupt.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}
