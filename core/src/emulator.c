#include "emulator.h"

#include <stdint.h>

#include "cpu.h"
#include "data_bus.h"
#include "rom.h"
#include "status_code.h"

status_code_t emulator_init(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  status_code_t status = STATUS_OK;

  cpu_init_param_t cpu_init_params = {
      .bus_read_fn = (cpu_bus_read_fn)data_bus_read,
      .bus_write_fn = (cpu_bus_write_fn)data_bus_write,
      .bus_resource = &emulator->bus_handle,
  };

  status = data_bus_init(&emulator->bus_handle);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(
      &emulator->bus_handle,
      SEGMENT_TYPE_ROM_BANK_0,
      (data_bus_segment_read_fn)rom_read,
      (data_bus_segment_write_fn)rom_write,
      &emulator->rom);
  RETURN_STATUS_IF_NOT_OK(status);

  status = data_bus_add_segment(
      &emulator->bus_handle,
      SEGMENT_TYPE_ROM_SWITCHABLE_BANK,
      (data_bus_segment_read_fn)rom_read,
      (data_bus_segment_write_fn)rom_write,
      &emulator->rom);
  RETURN_STATUS_IF_NOT_OK(status);

  status = cpu_init(&emulator->cpu_state, &cpu_init_params);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

status_code_t emulator_cleanup(emulator_t *const emulator)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(emulator);

  return rom_unload(&emulator->rom);
}
