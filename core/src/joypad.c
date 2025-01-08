#include "joypad.h"

#include <stdint.h>

#include "bus_interface.h"
#include "callback.h"
#include "status_code.h"

#define DPAD_SELECT (1 << 0)
#define BUTTON_SELECT (1 << 1)

static status_code_t joypad_read(void *const resource, uint16_t const __attribute__((unused)) address, uint8_t *const data);
static status_code_t joypad_write(void *const resource, uint16_t const __attribute__((unused)) address, uint8_t const data);
static status_code_t joypad_update_cb(void *const ctx, const void *arg);

status_code_t joypad_init(joypad_handle_t *const joypad)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(joypad);

  status_code_t status = STATUS_OK;

  joypad->key_select = 0x03;
  joypad->key_state = 0xFF;

  status = callback_init(&joypad->key_update_callback, joypad_update_cb, joypad);
  RETURN_STATUS_IF_NOT_OK(status);

  status = bus_interface_init(&joypad->bus_interface, joypad_read, joypad_write, joypad);
  RETURN_STATUS_IF_NOT_OK(status);

  return STATUS_OK;
}

static status_code_t joypad_update_cb(void *const ctx, const void *arg)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(ctx);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(arg);

  joypad_handle_t *const joypad = (joypad_handle_t *)ctx;
  joypad_key_update_event_t const *update = (joypad_key_update_event_t *)arg;

  switch (update->state)
  {
  case KEY_PRESSED:
    joypad->key_state &= ~(update->key);
    break;
  case KEY_RELEASED:
    joypad->key_state |= update->key;
    break;
  default:
    break;
  }

  return STATUS_OK;
}

static status_code_t joypad_read(void *const resource, uint16_t const __attribute__((unused)) address, uint8_t *const data)
{
  joypad_handle_t *const joypad = (joypad_handle_t *)resource;
  VERIFY_PTR_RETURN_ERROR_IF_NULL(joypad);

  *data = 0xF;

  if ((joypad->key_select & DPAD_SELECT) == 0)
  {
    *data = joypad->key_state & 0x0F;
  }
  else if ((joypad->key_select & BUTTON_SELECT) == 0)
  {
    *data = (joypad->key_state >> 4) & 0x0F;
  }

  *data |= ((joypad->key_select << 4) & 0xF0) | 0xC0;
  return STATUS_OK;
}

static status_code_t joypad_write(void *const resource, uint16_t const __attribute__((unused)) address, uint8_t const data)
{
  joypad_handle_t *const joypad = (joypad_handle_t *)resource;
  VERIFY_PTR_RETURN_ERROR_IF_NULL(joypad);

  joypad->key_select = (data >> 4) & 0x3;

  return STATUS_OK;
}
