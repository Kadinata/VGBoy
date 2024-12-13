#ifndef __STATUS_CODE_H__
#define __STATUS_CODE_H__

typedef enum
{
  STATUS_OK = 0,
  STATUS_ERR_GENERIC,
  STATUS_ERR_NULL_PTR,
  STATUS_ERR_INVALID_ARG,
  STATUS_ERR_UNDEFINED_INST,
} status_code_t;

#define VERIFY_PTR_RETURN_ERROR_IF_NULL(p) \
  if (p == NULL)                           \
  {                                        \
    return STATUS_ERR_NULL_PTR;            \
  }

#define RETURN_STATUS_IF_NOT_OK(status) \
  if (status != STATUS_OK)              \
  {                                     \
    return status;                      \
  }

#endif /* __STATUS_CODE_H__ */
