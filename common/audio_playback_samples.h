#ifndef __AUDIO_PLAYBACK_SAMPLES_H__
#define __AUDIO_PLAYBACK_SAMPLES_H__

#include <stdint.h>

typedef struct
{
  uint8_t *data;
  int32_t length;
  int32_t sample_rate_hz;
  float volume_adjust;
} audio_playback_samples_t;

#endif /* __AUDIO_PLAYBACK_SAMPLES_H__ */
