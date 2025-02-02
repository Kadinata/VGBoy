#ifndef __SAVE_STATE_H__
#define __SAVE_STATE_H__

#include <stdint.h>

#include "status_code.h"
#include "emulator.h"

status_code_t save_game_state(emulator_t *const emulator, const uint8_t slot_num);
status_code_t load_game_state(emulator_t *const emulator, const uint8_t slot_num);

#endif /* __SAVE_STATE_H__ */
