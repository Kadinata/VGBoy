#ifndef __SAVE_STATE_H__
#define __SAVE_STATE_H__

#include <stdint.h>

#include "status_code.h"
#include "emulator.h"

typedef enum
{
  MODE_SAVE_STATE,
  MODE_LOAD_STATE,
} game_state_mode_t;

status_code_t request_save_state(const uint8_t slot_num, const game_state_mode_t mode);
status_code_t handle_save_request(emulator_t *const emulator);

#endif /* __SAVE_STATE_H__ */
