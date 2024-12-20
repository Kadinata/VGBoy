#include <stdint.h>
#include <stdio.h>
#include "status_code.h"
#include "data_bus.h"
#include "rom.h"
#include "cpu.h"
#include "logging.h"
#include "emulator.h"
#include <unistd.h>

int main(int __attribute__((unused)) argc, char **argv)
{
  status_code_t status;
  emulator_t emulator = {0};

  status = emulator_init(&emulator);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init emulator: %d", status);
    return -status;
  }

  status = rom_load(&emulator.rom, argv[1]);
  if (status != STATUS_OK)
  {
    return -status;
  }

  for (uint16_t i = 0; i < 100; i++)
  {
    status = cpu_emulation_cycle(&emulator.cpu_state);
    if (status != STATUS_OK)
    {
      break;
    }
  }

  emulator_cleanup(&emulator);
  Log_I("Exiting: %d", status);
  return -status;
}
