#ifndef __STATUS_CODE_H__
#define __STATUS_CODE_H__

#include <stddef.h>

typedef enum
{
  STATUS_OK = 0,
  STATUS_ERR_GENERIC,
  STATUS_ERR_NULL_PTR,
  STATUS_ERR_NO_MEMORY,
  STATUS_ERR_INVALID_ARG,
  STATUS_ERR_UNDEFINED_INST,
  STATUS_ERR_FILE_NOT_FOUND,
  STATUS_ERR_NOT_INITIALIZED,
  STATUS_ERR_ALREADY_INITIALIZED,
  STATUS_ERR_ALREADY_FREED,
  STATUS_ERR_CHECKSUM_FAILURE,
  STATUS_ERR_ADDRESS_OUT_OF_BOUND,
} status_code_t;

#define VERIFY_COND_RETURN_STATUS_IF_TRUE(cond, status) \
  if (cond)                                             \
  {                                                     \
    return status;                                      \
  }

#define VERIFY_PTR_RETURN_STATUS_IF_NULL(p, status) VERIFY_COND_RETURN_STATUS_IF_TRUE((p == NULL), status)
#define VERIFY_PTR_RETURN_ERROR_IF_NULL(p) VERIFY_PTR_RETURN_STATUS_IF_NULL(p, STATUS_ERR_NULL_PTR)
#define RETURN_STATUS_IF_NOT_OK(status) VERIFY_COND_RETURN_STATUS_IF_TRUE((status != STATUS_OK), status)

#endif /* __STATUS_CODE_H__ */
