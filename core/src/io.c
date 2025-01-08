#include "io.h"

#include <stdint.h>

#include "bus_interface.h"
#include "logging.h"
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
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->lcd_bus_interface, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->int_bus_interface, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->joypad_bus_interface, STATUS_ERR_INVALID_ARG);
  VERIFY_PTR_RETURN_STATUS_IF_NULL(init_param->timer_bus_interface, STATUS_ERR_INVALID_ARG);

  io_handle->dma_handle = init_param->dma_handle;
  io_handle->int_bus_interface = init_param->int_bus_interface;
  io_handle->lcd_bus_interface = init_param->lcd_bus_interface;
  io_handle->joypad_bus_interface = init_param->joypad_bus_interface;
  io_handle->timer_bus_interface = init_param->timer_bus_interface;

  io_handle->timer_bus_interface->offset = 0x0004;
  io_handle->lcd_bus_interface->offset = 0x0040;

  return bus_interface_init(&io_handle->bus_interface, io_read, io_write, io_handle);
}

static status_code_t io_read(void *const resource, uint16_t const address, uint8_t *const data)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(resource);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(data);

  status_code_t status = STATUS_OK;
  io_handle_t *io_handle = (io_handle_t *)resource;

  switch (address)
  {
  case 0x0000:
    status = bus_interface_read(io_handle->joypad_bus_interface, address, data);
    break;
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
    status = bus_interface_read(io_handle->timer_bus_interface, address, data);
    break;
  case 0x000F:
    status = bus_interface_read(io_handle->int_bus_interface, address + io_handle->bus_interface.offset, data);
    break;
  case 0x0040:
  case 0x0041:
  case 0x0042:
  case 0x0043:
  case 0x0044:
  case 0x0045:
  case 0x0047:
  case 0x0048:
  case 0x0049:
  case 0x004A:
  case 0x004B:
    status = bus_interface_read(io_handle->lcd_bus_interface, address, data);
    break;
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
  case 0x0000:
    status = bus_interface_write(io_handle->joypad_bus_interface, address, data);
    break;
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
    status = bus_interface_write(io_handle->timer_bus_interface, address, data);
    break;
  case 0x000F:
    status = bus_interface_write(io_handle->int_bus_interface, address + io_handle->bus_interface.offset, data);
    break;
  case 0x0040:
  case 0x0041:
  case 0x0042:
  case 0x0043:
  case 0x0044:
  case 0x0045:
  case 0x0047:
  case 0x0048:
  case 0x0049:
  case 0x004A:
  case 0x004B:
    status = bus_interface_write(io_handle->lcd_bus_interface, address, data);
    break;
  case 0x0046:
    status = dma_start(io_handle->dma_handle, data);
    break;
  default:
    break;
  }

  return status;
}
