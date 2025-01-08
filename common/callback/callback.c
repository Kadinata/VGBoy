#include "callback.h"

status_code_t callback_init(callback_t *const callback, callback_fn_t const callback_fn, void *const ctx)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback);
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback_fn);

  callback->callback_fn = callback_fn;
  callback->callback_ctx = ctx;

  return STATUS_OK;
}

status_code_t callback_call(callback_t *const callback, const void *arg)
{
  VERIFY_PTR_RETURN_ERROR_IF_NULL(callback);
  VERIFY_COND_RETURN_STATUS_IF_TRUE(callback->callback_fn == NULL, STATUS_ERR_NOT_INITIALIZED);

  return callback->callback_fn(callback->callback_ctx, arg);
}
