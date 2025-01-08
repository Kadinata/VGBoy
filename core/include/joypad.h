#ifndef __DMG_JOYPAD_H__
#define __DMG_JOYPAD_H__

#include <stdint.h>

#include "bus_interface.h"
#include "callback.h"
#include "status_code.h"

/** TODO: Joypad interrupt handling */

typedef enum __attribute__((packed))
{
  KEY_RIGHT = (1 << 0),
  KEY_LEFT = (1 << 1),
  KEY_UP = (1 << 2),
  KEY_DOWN = (1 << 3),
  KEY_A = (1 << 4),
  KEY_B = (1 << 5),
  KEY_SELECT = (1 << 6),
  KEY_START = (1 << 7),
} joypad_key_mask_t;

typedef enum
{
  KEY_PRESSED = 0,
  KEY_RELEASED = 1,
} joypad_key_state_t;

typedef struct
{
  joypad_key_mask_t const key;
  joypad_key_state_t const state;
} joypad_key_update_event_t;

typedef struct
{
  uint8_t key_state;
  uint8_t key_select;
  callback_t key_update_callback;
  bus_interface_t bus_interface;
} joypad_handle_t;

status_code_t joypad_init(joypad_handle_t *const joypad);

#endif /* __DMG_JOYPAD_H__ */
