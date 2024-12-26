#include "io.h"

#include <stdint.h>

#include "bus_interface.h"
#include "logging.h"
#include "timer.h"
#include "interrupt.h"
#include "dma.h"
#include "status_code.h"
#include "debug_serial.h"

static status_code_t io_read(void *const resource, uint16_t const address, uint8_t *const data);
static status_code_t io_write(void *const resource, uint16_t const address, uint8_t const data);

status_code_t io_init(io_handle_t *const io_handle, io_init_param_t *const init_param)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(io_handle)
  VERIFY_PTR_RETURN_ERROR_IF_NULL(init_param)
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->dma_handle, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->int_handle, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->timer_handle, STATUS_ERR_INVALID_ARG);

  io_handle->dma_handle = init_param->dma_handle;
  io_handle->int_handle = init_param->int_handle;
  io_handle->timer_handle = init_param->timer_handle;

  io_handle->bus_interface.read = io_read;
  io_handle->bus_interface.write = io_write;
  io_handle->bus_interface.resource = io_handle;

  io_handle->timer_handle->bus_interface.offset = 0x0004;

  return STATUS_OK;
}

static status_code_t io_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  io_handle_t *io_handle = (io_handle_t *)resource;

  status_code_t status = STATUS_OK;
  static uint8_t ly = 0;

  switch (address)
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
    status = bus_interface_read(&io_handle->timer_handle->bus_interface, address, data);
    break;
  case 0x000F:
    *data = io_handle->int_handle->regs.irf; // TODO: create function
    break;
  case 0x0044:
    *data = ly++;
    return status;
  default:
    break;
  }

  return status;
}

static status_code_t io_write(void *const resource, uint16_t const address, uint8_t const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);

  status_code_t status = STATUS_OK;
  io_handle_t *io_handle = (io_handle_t *)resource;

  switch (address)
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
    status = bus_interface_write(&io_handle->timer_handle->bus_interface, address, data);
    break;
  case 0x000F:
    io_handle->int_handle->regs.irf = data; // TODO: create function
    break;
  case 0x0046:
    status = dma_start(io_handle->dma_handle, data);
    break;
  default:
    break;
  }

  return status;
}
