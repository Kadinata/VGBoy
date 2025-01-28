#include "display.h"

#include <stdint.h>
#include <SDL2/SDL.h>

#include "logging.h"
#include "callback.h"
#include "status_code.h"
#include "bus_interface.h"
#include "tile_debug_window.h"
#include "main_window.h"
#include "fps_sync.h"
#include "color.h"

typedef struct
{
  fps_sync_handle_t fps_sync_handle;
  ppu_handle_t *ppu;
  uint32_t prev_ppu_frame;
} display_handle_t;

static display_handle_t display_handle;

static status_code_t handle_fps_sync(void *const ctx, const void __attribute__((unused)) * arg)
{
  fps_sync_handle_t *const handle = (fps_sync_handle_t *)ctx;
  return fps_sync(handle);
}

status_code_t display_init(bus_interface_t const data_bus_interface, ppu_handle_t *const ppu_handle)
{
  Log_I("Initializing the display module...");

  VERIFY_PTR_RETURN_ERROR_IF_NULL(ppu_handle);

  status_code_t status = STATUS_OK;
  int16_t init_result;
  if ((init_result = SDL_InitSubSystem(SDL_INIT_VIDEO)) != 0)
  {
    Log_E("Failed to inittialize SDL Video Subsystem (%d)", init_result);
    return STATUS_ERR_GENERIC;
  }

  callback_t fps_sync_callback = {0};

  display_handle.ppu = ppu_handle;
  display_handle.prev_ppu_frame = 0;

  status = tile_debug_window_init(data_bus_interface);
  RETURN_STATUS_IF_NOT_OK(status);

  status = main_window_init(ppu_handle->video_buffer.buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
  RETURN_STATUS_IF_NOT_OK(status);

  status = callback_init(&fps_sync_callback, handle_fps_sync, (void *)&display_handle.fps_sync_handle);
  RETURN_STATUS_IF_NOT_OK(status);

  status = fps_sync_init(&display_handle.fps_sync_handle, 60);
  RETURN_STATUS_IF_NOT_OK(status);

  status = ppu_register_fps_sync_callback(ppu_handle, &fps_sync_callback);
  RETURN_STATUS_IF_NOT_OK(status);

  Log_I("Display module successfully initialized.");
  return STATUS_OK;
}

void display_cleanup(void)
{
  Log_I("Cleaning up the display module.");
  main_window_cleanup();
  tile_debug_window_cleanup();
  SDL_Quit();
}

void update_display(void)
{
  if (display_handle.prev_ppu_frame != display_handle.ppu->current_frame)
  {
    main_window_update();
    tile_debug_window_update();
  }
  display_handle.prev_ppu_frame = display_handle.ppu->current_frame;
}
