#include "emulator.h"

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "ram.h"
#include "oam.h"
#include "io.h"
#include "dma.h"
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
    }

    status = dma_tick(&emulator->dma);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return status;
}

status_code_t emulator_init(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  status_code_t status = STATUS_OK;

  emulator->ram.wram.offset = 0xC000;
  emulator->ram.vram.offset = 0x8000;
  emulator->ram.hram.offset = 0xFF80;
  emulator->oam.offset = 0xFE00;
  emulator->io.offset = 0xFF00;
  emulator->tmr.addr_offset = 0xFF04;

  status = data_bus_init(&emulator->bus_handle);
  RETURN_STATUS_IF_NOT_OK(status);

  bus_interface_t bus_interface_rom = (bus_interface_t){
      .read = (bus_read_fn)rom_read,
      .write = (bus_write_fn)rom_write,
      .resource = &emulator->rom,
  };

  bus_interface_t bus_interface_ram = (bus_interface_t){
      .read = (bus_read_fn)ram_read,
      .write = (bus_write_fn)ram_write,
      .resource = &emulator->ram,
  };

  bus_interface_t bus_interface_oam = (bus_interface_t){
      .read = (bus_read_fn)oam_read,
      .write = (bus_write_fn)oam_write,
      .resource = &emulator->oam,
  };

  bus_interface_t bus_interface_data_bus = (bus_interface_t){
      .read = (bus_read_fn)data_bus_read,
      .write = (bus_write_fn)data_bus_write,
      .resource = &emulator->bus_handle,
  };

  bus_interface_t bus_interface_io = (bus_interface_t){
      .read = (bus_read_fn)io_read,
      .write = (bus_write_fn)io_write,
      .resource = &emulator->io,
  };

  bus_interface_t bus_interface_ie_reg = (bus_interface_t){
      .read = (bus_read_fn)int_read_enabled_mask,
      .write = (bus_write_fn)int_write_enabled_mask,
      .resource = &emulator->interrupt,
  };

  cpu_init_param_t cpu_init_params = {
      .bus_interface = bus_interface_data_bus,
      .int_handle = &emulator->interrupt,
      .sync_handle = &emulator->sync_handle,
  };

  io_init_param_t io_init_params = {
      .int_handle = &emulator->interrupt,
      .timer_handle = &emulator->tmr,
  };

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_BANK_0, bus_interface_rom);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_ROM_SWITCHABLE_BANK, bus_interface_rom);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_WRAM, bus_interface_ram);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_VRAM, bus_interface_ram);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_OAM, bus_interface_oam);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_HRAM, bus_interface_ram);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IO_REG, bus_interface_io);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(&emulator->bus_handle, SEGMENT_TYPE_IE_REG, bus_interface_ie_reg);
  RETURN_STATUS_IF_NOT_OK(status);

  status = timer_init(&emulator->tmr, &emulator->interrupt);
  RETURN_STATUS_IF_NOT_OK(status);

  status = io_init(&emulator->io, &io_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = dma_init(&emulator->dma, bus_interface_data_bus);
  RETURN_STATUS_IF_NOT_OK(status);

  status = cpu_init(&emulator->cpu_state, &cpu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  status = timing_sync_init(&emulator->sync_handle, sync_callback_handler, (void *)emulator);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t emulator_cleanup(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  return rom_unload(&emulator->rom);
}
