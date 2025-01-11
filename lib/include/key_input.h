#ifndef __KEY_INPUT_H__
#define __KEY_INPUT_H__

#include <stdint.h>

#include "callback.h"
#include "joypad.h"
#include "status_code.h"

status_code_t key_input_init(callback_t *const key_update_cb);
status_code_t key_input_read(void);

#endif /* __KEY_INPUT_H__ */
