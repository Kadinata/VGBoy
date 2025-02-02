#include <stdint.h>
#include <stdio.h>

#include "status_code.h"
#include "cpu.h"
#include "file_manager.h"
#include "snapshot.h"
#include "logging.h"
#include "emulator.h"
#include "audio.h"
#include "display.h"
#include "key_input.h"

#include <pthread.h>
#include <unistd.h>

void *cpu_run(void *p)
{
  emulator_t *const emulator = (emulator_t *)p;

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

    status = handle_snapshot_request(emulator);
    if (status != STATUS_OK)
    {
      Log_E("An error occurred while handling game state request: %d", status);
      break;
    }
  }

  emulator->state = EMU_MODE_STOPPED;
  return 0;
}

static status_code_t init(emulator_t *const emulator, const char *rom_file)
{
  status_code_t status = STATUS_OK;

  status = emulator_init(emulator);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init emulator: %d", status);
    return status;
  }

  status = setup_mbc_callbacks(&emulator->mbc);
  if (status != STATUS_OK)
  {
    Log_E("Failed to setup MBC callbacks: %d", status);
    return status;
  }

  status = load_cartridge(&emulator->mbc, rom_file);
  if (status != STATUS_OK)
  {
    Log_E("Failed to load cartridge: %d", status);
    return status;
  }

  status = display_init(emulator->bus_handle.bus_interface, &emulator->ppu);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init display: %d", status);
    return status;
  }

  status = audio_init(&emulator->apu.playback_cb);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init audio device: %d", status);
    return status;
  }

  status = key_input_init(&emulator->joypad.key_update_callback);
  if (status != STATUS_OK)
  {
    Log_E("Failed to init key input: %d", status);
    return status;
  }

  return STATUS_OK;
}

static void cleanup(emulator_t *const emulator)
{
  audio_cleanup();
  display_cleanup();
  unload_cartridge(&emulator->mbc);
}

int main(int __attribute__((unused)) argc, char **argv)
{
  status_code_t status;
  emulator_t emulator = {0};

  status = init(&emulator, argv[1]);
  if (status != STATUS_OK)
  {
    cleanup(&emulator);
    return -status;
  }

  pthread_t t1;
  if (pthread_create(&t1, NULL, cpu_run, &emulator))
  {
    Log_E("Failed to start main thread");
    return -1;
  }

  while (emulator.state == EMU_MODE_RUNNING)
  {
    usleep(1000);
    status = key_input_read();

    if (status == STATUS_REQ_EXIT)
    {
      break;
    }

    update_display();
  }

  Log_I("Stopping");

  emulator_stop(&emulator);
  pthread_join(t1, NULL);

  cleanup(&emulator);

  Log_I("Exiting: %d", status);
  return -status;
}
