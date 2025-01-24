#include <stdint.h>
#include <stdio.h>
#include "status_code.h"
#include "cpu.h"
#include "logging.h"
#include "emulator.h"
#include "audio.h"
#include "display.h"
#include "key_input.h"

#include <pthread.h>
#include <unistd.h>

void *cpu_run(void *p)
{
  status_code_t status;
  emulator_t *const emulator = (emulator_t *)p;

  while (emulator->running)
  {
    status = cpu_emulation_cycle(&emulator->cpu_state);
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

  return 0;
}

void cleanup(emulator_t *const emulator)
{
  display_cleanup();
  audio_cleanup();
  emulator_cleanup(emulator);
}

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

  status = mbc_load_rom(&emulator.mbc, argv[1]);
  if (status != STATUS_OK)
  {
    return -status;
  }

  status = display_init(emulator.bus_handle.bus_interface, &emulator.ppu);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init display: %d", status);
    cleanup(&emulator);
    return -status;
  }

  status = audio_init(&emulator.apu.playback_cb);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init audio device: %d", status);
    cleanup(&emulator);
    return -status;
  }

  status = key_input_init(&emulator.joypad.key_update_callback);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init key input: %d", status);
    cleanup(&emulator);
    return -status;
  }

  pthread_t t1;
  if (pthread_create(&t1, NULL, cpu_run, &emulator))
  {
    Log_E("Failed to start main thread");
    return -1;
  }

  while (1)
  {
    usleep(1000);
    status = key_input_read();

    if (status == STATUS_REQ_EXIT)
    {
      break;
    }
  }

  Log_I("Stopping");

  emulator.running = false;
  pthread_join(t1, NULL);

  cleanup(&emulator);

  Log_I("Exiting: %d", status);
  return -status;
}
