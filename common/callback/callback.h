#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <stdint.h>
#include "status_code.h"

typedef status_code_t (*callback_fn_t)(void *const ctx, const void *arg);

typedef struct
{
  callback_fn_t callback_fn;
  void *callback_ctx;
} callback_t;

status_code_t callback_init(callback_t *const callback, callback_fn_t const callback_fn, void *const ctx);
status_code_t callback_call(callback_t *const callback, const void *arg);

#endif /* __CALLBACK_H__ */
