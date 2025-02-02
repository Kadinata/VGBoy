#include "key_input.h"

#include <stdint.h>
#include <SDL2/SDL.h>

#include "callback.h"
#include "joypad.h"
#include "save_state.h"
#include "status_code.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef struct
{
  callback_t *update_cb;
} key_input_handle_t;

static key_input_handle_t key_input_handle;

static uint8_t const scancode_key_mapping[] = {
    [SDL_SCANCODE_W] = KEY_UP,
    [SDL_SCANCODE_A] = KEY_LEFT,
    [SDL_SCANCODE_S] = KEY_DOWN,
    [SDL_SCANCODE_D] = KEY_RIGHT,
    [SDL_SCANCODE_K] = KEY_B,
    [SDL_SCANCODE_L] = KEY_A,
    [SDL_SCANCODE_RETURN] = KEY_START,
    [SDL_SCANCODE_BACKSPACE] = KEY_SELECT,
};

static inline uint16_t get_key_from_scancode(SDL_Scancode scancode);
static inline uint8_t get_slot_num_from_scancode(SDL_Scancode scancode);
static inline status_code_t should_quit(SDL_Event event);
static status_code_t update_key_press(SDL_Event event);
static status_code_t handle_save_state_requests(SDL_Event event);

status_code_t key_input_init(callback_t *const key_update_cb)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(key_update_cb);

  key_input_handle.update_cb = key_update_cb;

  return STATUS_OK;
}

status_code_t key_input_read(void)
{
  SDL_Event event;
  status_code_t status = STATUS_OK;

  while (SDL_PollEvent(&event))
  {
    status = should_quit(event);
    RETURN_STATUS_IF_NOT_OK(status);

    status = update_key_press(event);
    RETURN_STATUS_IF_NOT_OK(status);

    status = handle_save_state_requests(event);
    RETURN_STATUS_IF_NOT_OK(status);
  }

  return status;
}

static inline uint16_t get_key_from_scancode(SDL_Scancode scancode)
{
  return (scancode < ARRAY_SIZE(scancode_key_mapping)) ? scancode_key_mapping[scancode] : 0;
}

static inline uint8_t get_slot_num_from_scancode(SDL_Scancode scancode)
{
  switch (scancode)
  {
  case SDL_SCANCODE_0:
    return 0;
  case SDL_SCANCODE_1:
    return 1;
  case SDL_SCANCODE_2:
    return 2;
  case SDL_SCANCODE_3:
    return 3;
  case SDL_SCANCODE_4:
    return 4;
  case SDL_SCANCODE_5:
    return 5;
  case SDL_SCANCODE_6:
    return 6;
  case SDL_SCANCODE_7:
    return 7;
  case SDL_SCANCODE_8:
    return 8;
  case SDL_SCANCODE_9:
    return 9;
  default:
    break;
  }
  return 0xFF;
}

static inline status_code_t should_quit(SDL_Event event)
{
  const uint8_t *keyboard_state = SDL_GetKeyboardState(NULL);

  if ((event.type == SDL_QUIT) || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) || keyboard_state[SDL_SCANCODE_ESCAPE])
  {
    return STATUS_REQ_EXIT;
  }

  return STATUS_OK;
}

static status_code_t handle_save_state_requests(SDL_Event event)
{

  status_code_t status = STATUS_OK;
  uint8_t slot_num = get_slot_num_from_scancode(event.key.keysym.scancode);

  if ((event.type != SDL_KEYUP) || (slot_num > 9))
  {
    return STATUS_OK;
  }
  else if (event.key.keysym.mod & KMOD_GUI)
  {
    status = request_save_state(slot_num, MODE_SAVE_STATE);
  }
  else if (event.key.keysym.mod & KMOD_SHIFT)
  {
    status = request_save_state(slot_num, MODE_LOAD_STATE);
  }

  return status;
}

static status_code_t update_key_press(SDL_Event event)
{
  joypad_key_state_t key_state;
  joypad_key_mask_t key = get_key_from_scancode(event.key.keysym.scancode);

  if (key == 0)
  {
    return STATUS_OK;
  }

  switch (event.type)
  {
  case SDL_KEYDOWN:
    key_state = KEY_PRESSED;
    break;

  case SDL_KEYUP:
    key_state = KEY_RELEASED;
    break;

  default:
    return STATUS_OK;
  }

  const joypad_key_update_event_t key_update = {
      .key = key,
      .state = key_state,
  };

  return callback_call(key_input_handle.update_cb, &key_update);
}
