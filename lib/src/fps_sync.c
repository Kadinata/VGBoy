#include "fps_sync.h"

#include <stdint.h>
#include <SDL2/SDL.h>

#include "logging.h"
#include "status_code.h"

status_code_t fps_sync_init(fps_sync_handle_t *const handle, uint32_t const target_frame_rate)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(target_frame_rate == 0, STATUS_ERR_INVALID_ARG);

  handle->actual_frame_rate = 0;
  handle->last_frame_timestamp = 0;
  handle->secondly_timestamp = 0;
  handle->frame_interval_sec = 1.0 / target_frame_rate;

  return STATUS_OK;
}

status_code_t fps_sync(fps_sync_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  uint64_t initial_timestamp = SDL_GetPerformanceCounter();
  uint64_t current_timestamp = initial_timestamp;
  double frame_interval = (current_timestamp - handle->last_frame_timestamp);
  frame_interval /= SDL_GetPerformanceFrequency();

  while (frame_interval < handle->frame_interval_sec)
  {
    current_timestamp = SDL_GetPerformanceCounter();
    frame_interval = (current_timestamp - handle->last_frame_timestamp);
    frame_interval /= SDL_GetPerformanceFrequency();
  }

  double time_delta = (initial_timestamp - handle->secondly_timestamp);
  time_delta /= SDL_GetPerformanceFrequency();

  if (time_delta >= 1.0)
  {
    Log_I("FPS: %u", handle->actual_frame_rate);
    handle->secondly_timestamp = initial_timestamp;
    handle->actual_frame_rate = 0;
  }

  handle->actual_frame_rate++;
  handle->last_frame_timestamp = SDL_GetPerformanceCounter();

  return STATUS_OK;
}
