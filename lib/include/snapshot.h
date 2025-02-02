#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include <stdint.h>

#include "status_code.h"
#include "emulator.h"

typedef enum
{
  MODE_SAVE_SNAPSHOT,
  MODE_LOAD_SNAPSHOT,
} game_state_mode_t;

status_code_t request_snapshot(const uint8_t slot_num, const game_state_mode_t mode);
status_code_t handle_snapshot_request(emulator_t *const emulator);

#endif /* __SNAPSHOT_H__ */
