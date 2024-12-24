#include "io.h"

#include <stdint.h>

#include "logging.h"
#include "timer.h"
#include "interrupt.h"
#include "status_code.h"
#include "debug_serial.h"

status_code_t io_init(io_handle_t *const io_handle, io_init_param_t *const init_param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle)
  VERIFY_PTR_RETURN_ERROR_IF_NULL(init_param)
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->int_handle, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->timer_handle, STATUS_ERR_INVALID_ARG);

  io_handle->int_handle = init_param->int_handle;
  io_handle->timer_handle = init_param->timer_handle;

  return STATUS_OK;
}

status_code_t io_read(io_handle_t *const io_handle, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  status_code_t status = STATUS_OK;

  switch (address - io_handle->offset)
  {
  case 0x0001:
    serial_read(0, data);
    break;
  case 0x0002:
    serial_read(1, data);
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
  default:
    break;
  }

  return status;
}

status_code_t io_write(io_handle_t *const io_handle, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle);

  status_code_t status = STATUS_OK;

  switch (address - io_handle->offset)
  {
  case 0x0001:
    serial_write(0, data);
    break;
  case 0x0002:
    serial_write(1, data);
    break;
  case 0x0004:
  case 0x0005:
  case 0x0006:
  case 0x0007:
    status = timer_write(io_handle->timer_handle, address, data);
    break;
  case 0x000F:
    io_handle->int_handle->int_requested_flag = data; // TODO: create function
    break;
  default:
    break;
  }

  return status;
}
