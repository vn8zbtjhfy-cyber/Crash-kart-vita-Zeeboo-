#ifndef CNK_VITA_AUDIO_H
#define CNK_VITA_AUDIO_H
#include "cnk_audio.h"
/* Native PS Vita SceAudioOut stereo PCM output (NOT Zeebo emulation).
 * Intended to be used from one dedicated audio thread. The mixer is not
 * thread-safe: never mutate voices concurrently with cnk_vita_audio_pump.
 * This adapter is not hardware-tested yet. */
#define CNK_VITA_AUDIO_FRAMES 512u

typedef struct {
 int port;
 cnk_audio_mixer mixer;
 int16_t buffer[CNK_VITA_AUDIO_FRAMES*2u];
} cnk_vita_audio;

int cnk_vita_audio_open(cnk_vita_audio *a);
/* Mix one block, then send it to the hardware; blocks until queued. */
int cnk_vita_audio_pump(cnk_vita_audio *a);
void cnk_vita_audio_close(cnk_vita_audio *a);
#endif
