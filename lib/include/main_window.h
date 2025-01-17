#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <stdint.h>

#include "status_code.h"

status_code_t main_window_init(uint32_t *const video_buffer);
void main_window_update(void);
void main_window_cleanup(void);

#endif /* __MAIN_WINDOW_H__ */
