#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdint.h>

#include "callback.h"
#include "status_code.h"

status_code_t audio_init(callback_t *const playback_cb);
void audio_cleanup(void);

#endif /* __AUDIO_H__ */
