#include "io.h"

#include <stdint.h>

#include "logging.h"
#include "timer.h"
#include "interrupt.h"
#include "status_code.h"

static char serial_data[2];

status_code_t io_read(io_handle_t *const io_handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  status_code_t status = STATUS_OK;

  switch (address - io_handle->offset)
  {
  case 0x0001:
    *data = serial_data[0];
    break;
  case 0x0002:
    *data = serial_data[1];
    break;
  case 0x0004:
  case 0x0005:
  case 0x0006:
  case 0x0007:
    status = timer_read(io_handle->timer_handle, address, data);
    break;
  case 0x000F:
    *data = io_handle->int_handle->int_requested_flag; // TODO: create function
    break;
  case 0x00FF:
    *data = io_handle->int_handle->int_enable_mask; // TODO: create function
    break;
  default:
    break;
  }

  Log_I("Reading IO from address 0x%04X", address);

  return status;
}

status_code_t io_write(io_handle_t *const io_handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle);

  status_code_t status = STATUS_OK;

  switch (address - io_handle->offset)
  {
  case 0x0001:
    serial_data[0] = data;
    break;
  case 0x0002:
    serial_data[1] = data;
  case 0x0004:
  case 0x0005:
  case 0x0006:
  case 0x0007:
    status = timer_write(io_handle->timer_handle, address, data);
    break;
  case 0x000F:
    io_handle->int_handle->int_requested_flag = data; // TODO: create function
    break;
  case 0x00FF:
    io_handle->int_handle->int_enable_mask = data; // TODO: create function
    break;
  default:
    break;
  }

  Log_I("Writing to IO address 0x%04X; data: 0x%02X", address, data);

  return status;
}
