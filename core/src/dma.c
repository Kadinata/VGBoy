#include "dma.h"

#include <stdint.h>
#include <string.h>

#include "bus_interface.h"
#include "status_code.h"

#define OAM_ADDR_OFFSET (0xFE00)

status_code_t dma_init(dma_handle_t *const handle, bus_interface_t const bus_interface)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface.read);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(bus_interface.write);

  memset(handle, 0, sizeof(dma_handle_t));
  memcpy(&handle->bus_interface, &bus_interface, sizeof(bus_interface_t));

  handle->state = DMA_IDLE;

  return STATUS_OK;
}

status_code_t dma_tick(dma_handle_t *const handle)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  status_code_t status = STATUS_OK;

  if (handle->state == DMA_IDLE)
  {
    return STATUS_OK;
  }

  if (handle->state == DMA_PREPARING)
  {
    handle->prep_delay--;
    handle->state = (handle->prep_delay) ? DMA_PREPARING : DMA_XFER_ACTIVE;
    return STATUS_OK;
  }

  uint8_t data;

  status = bus_interface_read(&handle->bus_interface, handle->starting_addr + handle->current_offset, &data);
  RETURN_STATUS_IF_NOT_OK(status);

  status = bus_interface_write(&handle->bus_interface, OAM_ADDR_OFFSET + handle->current_offset, data);
  RETURN_STATUS_IF_NOT_OK(status);

  handle->current_offset++;

  if (handle->current_offset == 0xA0)
  {
    handle->state = DMA_IDLE;
  }

  return STATUS_OK;
}

status_code_t dma_start(dma_handle_t *const handle, uint8_t const offset)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(handle);

  handle->state = DMA_PREPARING;
  handle->starting_addr = offset << 8;
  handle->current_offset = 0;
  handle->prep_delay = 2;

  return STATUS_OK;
}
