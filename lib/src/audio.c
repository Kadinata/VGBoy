#include "audio.h"

#include <stdint.h>
#include <SDL2/SDL.h>

#include "audio_playback_samples.h"
#include "callback.h"
#include "logging.h"
#include "status_code.h"

typedef struct
{
  SDL_AudioDeviceID audio_device;
  callback_t *playback_cb;
} audio_handle_t;

static audio_handle_t audio_handle;

static void audio_callback(void __attribute__((unused)) *userdata, uint8_t *audio_buffer, int len)
{
  const audio_playback_samples_t playback_samples = {
      .data = audio_buffer,
      .length = len,
  };

  if (audio_handle.playback_cb)
  {
    callback_call(audio_handle.playback_cb, &playback_samples);
  }
}

status_code_t audio_init(callback_t *const playback_cb)
{
  Log_I("Initializing the audio module...");

  int16_t init_result;
  if ((init_result = SDL_InitSubSystem(SDL_INIT_AUDIO)) != 0)
  {
    Log_E("Failed to inittialize SDL Audio Subsystem (%d)", init_result);
    return STATUS_ERR_GENERIC;
  }

  SDL_AudioSpec desired_spec = (SDL_AudioSpec){
      .freq = 44000,
      .format = AUDIO_S16LSB,
      .channels = 2,
      .samples = 512,
      .callback = audio_callback,
      .userdata = NULL,
  };

  SDL_AudioSpec obtained_spec;

  audio_handle.audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
  audio_handle.playback_cb = playback_cb;

  status_code_t status = STATUS_OK;

  if (audio_handle.audio_device == 0)
  {
    Log_E("Failed to open audio device.");
    return STATUS_ERR_GENERIC;
  }

  if (desired_spec.freq != obtained_spec.freq)
  {
    Log_E("Failed to obtain the desired audio sample rate. Desired: %d Hz; obtained: %d Hz", desired_spec.freq, obtained_spec.freq);
    status = STATUS_ERR_GENERIC;
  }

  if (desired_spec.format != obtained_spec.format)
  {
    Log_E("Failed to obtain the desired audio format. Desired: 0x%04X; obtained: 0x%04X", desired_spec.format, obtained_spec.format);
    status = STATUS_ERR_GENERIC;
  }

  if (desired_spec.channels != obtained_spec.channels)
  {
    Log_E("Failed to obtain the desired number of audio channels. Desired: %d; obtained: %d", desired_spec.channels, obtained_spec.channels);
    status = STATUS_ERR_GENERIC;
  }

  if (desired_spec.samples != obtained_spec.samples)
  {
    Log_E("Failed to obtain the desired number of audio samples. Desired: %d; obtained: %d", desired_spec.samples, obtained_spec.samples);
    status = STATUS_ERR_GENERIC;
  }

  if (status == STATUS_OK)
  {
    Log_I("Audio module successfully initialized.");
  }

  SDL_PauseAudioDevice(audio_handle.audio_device, 0);

  return status;
}

void audio_cleanup(void)
{
  Log_I("Cleaning up the audio module.");
  SDL_PauseAudioDevice(audio_handle.audio_device, 1);
  SDL_CloseAudioDevice(audio_handle.audio_device);
}
