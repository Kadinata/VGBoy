#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

/** Structure to encode color in RGBA format */
typedef union
{
  struct __attribute__((packed))
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  uint32_t as_hex;
} color_rgba_t;

#endif /* __COLOR_H__ */
