#ifndef __FPS_SYNC_H__
#define __FPS_SYNC_H__

#include <stdint.h>
#include "status_code.h"

/**
 * Frame rate synchronizer data structure to keep track of
 * frame rate measurement and other timing information.
 */
typedef struct
{
  double frame_interval_sec;
  uint64_t last_frame_timestamp; /** Timestamp of the previous frame rendering */
  uint64_t secondly_timestamp;   /** Timestamp to keep track of when a second has elapsed */
  uint16_t actual_frame_rate;    /** The actual number of frames rendered in a second */
} fps_sync_handle_t;

/**
 * Initializes a frame rate synchronizer.
 *
 * @param handle Pointer to a frame rate synchronizer to initialize
 * @param target_frame_rate Desired frame rate in FPS that the synchronizer will try to match; must be greater than 0
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t fps_sync_init(fps_sync_handle_t *const handle, uint32_t const target_frame_rate);

/**
 * Synchronizes each frame to the desired frame rate setting of the synchronizer.
 *
 * This function must be called after each frame finishes rendering, i.e. when the
 * PPU transitions into the V-Blank mode. This function adds necessary delays to
 * achieve the target frame rate.
 *
 * @param handle Pointer to a frame rate synchronizer to synchronize to
 *
 * @return `STATUS_OK` if initialization is successful, otherwise appropriate error code.
 */
status_code_t fps_sync(fps_sync_handle_t *const handle);

#endif /* __FPS_SYNC_H__ */
