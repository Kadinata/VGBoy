#include "timing_sync.h"

#include <stdint.h>
#include "status_code.h"

status_code_t timing_sync_init(timing_sync_handle_t *const sync_handle, sync_handle_callback_fn const callback, void *const callback_ctx)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(sync_handle);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback_ctx);

  VERIFY_COND_RETURN_STATUS_IF_TRUE((sync_handle->sync_callback != NULL), STATUS_ERR_ALREADY_INITIALIZED);
  VERIFY_COND_RETURN_STATUS_IF_TRUE((sync_handle->callback_ctx != NULL), STATUS_ERR_ALREADY_INITIALIZED);

  sync_handle->sync_callback = callback;
  sync_handle->callback_ctx = callback_ctx;

  return STATUS_OK;
}

status_code_t timing_m_cycle_sync(timing_sync_handle_t *const sync_handle, uint8_t const m_cycle_count)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(sync_handle);

  status_code_t status = STATUS_OK;

  if (sync_handle->sync_callback)
  {
    status = sync_handle->sync_callback(sync_handle->callback_ctx, m_cycle_count);
  }

  return status;
}
