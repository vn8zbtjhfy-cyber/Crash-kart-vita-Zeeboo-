#ifndef CNK_AUDIO_H
#define CNK_AUDIO_H
#include <stddef.h>
#include <stdint.h>
#include "fufs.h"

/* Portable native game's PCM audio engine. This does NOT execute BREW.
 * Source WAV bytes are borrowed; keep them alive while voices are active.
 * Audio cues and file names are NOT assigned until verified from the game.
 */
#define CNK_AUDIO_MAX_VOICES 16u

typedef struct {
 const uint8_t *pcm;
 size_t pcm_bytes;
 uint32_t sample_rate;
 uint32_t frame_count;
 uint16_t channels;
 uint16_t bits_per_sample;
 uint16_t frame_bytes;
} cnk_wav;

typedef struct {
 uint8_t *bytes;
 size_t bytes_length;
 cnk_wav wav;
} cnk_wav_asset;

typedef struct {
 cnk_wav source;
 double frame_position;
 float volume;
 uint8_t active;
 uint8_t loop;
 uint8_t paused; /* native pause: freeze PCM cursor without losing sample */
} cnk_audio_voice;

typedef struct {
 cnk_audio_voice voices[CNK_AUDIO_MAX_VOICES];
 uint32_t sample_rate;
 float master_volume;
} cnk_audio_mixer;

/* Parses uncompressed RIFF PCM, 8/16 bit, 1/2 channels. Returns 0 on success. */
int cnk_wav_open(const uint8_t *data,size_t size,cnk_wav *out);
/* Copies a RIFF PCM source or decodes IMA ADPCM (WAVE tag 17) to PCM16.
 * Always owns the bytes, so the caller may release the input afterward.
 * The IMA decoder is adapted from Zeebx (GPL-2.0-or-later). */
int cnk_wav_asset_open_memory(cnk_wav_asset *out,const uint8_t *data,size_t size);
/* Owns extracted audio; keep loaded until all mixer voices stop. */
int cnk_wav_asset_load(cnk_wav_asset *out,const fufs_file *vfs,uint32_t index);
void cnk_wav_asset_free(cnk_wav_asset *asset);
int cnk_audio_init(cnk_audio_mixer *m,uint32_t sample_rate);
/* Starts a voice; returns handle in [0,15], or negative error. */
int cnk_audio_play(cnk_audio_mixer *m,const cnk_wav *src,float volume,int loop);
int cnk_audio_stop(cnk_audio_mixer *m,unsigned voice);
/* Pauses one active voice, keeping PCM and cursor intact. Call under mixer mutex. */
int cnk_audio_pause(cnk_audio_mixer *m,unsigned voice,int pause);
/* Produces interleaved signed 16-bit stereo, independent of input WAV rate.
 * No dynamic allocation on audio callback. Frames must fit size_t/2.
 * All currently active sources need to be kept alive during render.
 */
int cnk_audio_render(cnk_audio_mixer *m,int16_t *out,size_t frames);
unsigned cnk_audio_active(const cnk_audio_mixer *m);
#endif
