#include "emulator.h"

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "ram.h"
#include "oam.h"
#include "io.h"
#include "dma.h"
#include "ppu.h"
#include "timer.h"
#include "timing_sync.h"
#include "logging.h"
#include "bus_interface.h"
#include "status_code.h"

static status_code_t sync_callback_handler(void *const ctx, uint8_t const m_cycle_count)
{
  emulator_t *const emulator = (emulator_t *)ctx;
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

static inline status_code_t module_init(emulator_t *const emulator)
{
  status_code_t status = STATUS_OK;

  cpu_init_param_t cpu_init_params = {
      .bus_interface = &emulator->bus_handle.bus_interface,
      .int_handle = &emulator->interrupt,
      .sync_handle = &emulator->sync_handle,
  };

  io_init_param_t io_init_params = {
      .dma_handle = &emulator->dma,
      .int_bus_interface = &emulator->interrupt.bus_interface,
      .lcd_bus_interface = &emulator->ppu.lcd.bus_interface,
      .timer_bus_interface = &emulator->tmr.bus_interface,
  };

  ppu_init_param_t ppu_init_params = {
      .bus_interface = &emulator->bus_handle.bus_interface,
      .interrupt = &emulator->interrupt,
  };

  status = data_bus_init(&emulator->bus_handle);
  RETURN_STATUS_IF_NOT_OK(status);

  status = ram_init(&emulator->ram);
  RETURN_STATUS_IF_NOT_OK(status);

  status = rom_init(&emulator->rom);
  RETURN_STATUS_IF_NOT_OK(status);

  status = timer_init(&emulator->tmr, &emulator->interrupt);
  RETURN_STATUS_IF_NOT_OK(status);

  status = io_init(&emulator->io, &io_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = dma_init(&emulator->dma, emulator->bus_handle.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = ppu_init(&emulator->ppu, &ppu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = cpu_init(&emulator->cpu_state, &cpu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = timing_sync_init(&emulator->sync_handle, sync_callback_handler, (void *)emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static inline status_code_t configure_data_bus(emulator_t *const emulator)
{
  status_code_t status = STATUS_OK;

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_BANK_0, emulator->rom.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_SWITCHABLE_BANK, emulator->rom.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_WRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_VRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_EXT_RAM, emulator->rom.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_OAM, emulator->ppu.oam.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_HRAM, emulator->ram.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IO_REG, emulator->io.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IE_REG, emulator->interrupt.bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

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
  emulator->interrupt.bus_interface.offset = 0xFF00;

  status = module_init(emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  status = configure_data_bus(emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t emulator_cleanup(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  return rom_unload(&emulator->rom);
}
