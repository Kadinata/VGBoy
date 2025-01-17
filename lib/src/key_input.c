#include "key_input.h"

#include <stdint.h>
#include <SDL2/SDL.h>

#include "callback.h"
#include "joypad.h"
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
static inline status_code_t should_quit(SDL_Event event);
static status_code_t update_key_press(SDL_Event event);

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
  }

  return status;
}

static inline uint16_t get_key_from_scancode(SDL_Scancode scancode)
{
  return (scancode < ARRAY_SIZE(scancode_key_mapping)) ? scancode_key_mapping[scancode] : 0;
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
