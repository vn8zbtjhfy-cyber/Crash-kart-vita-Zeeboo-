/* Adaptation of the IMA ADPCM decoder in Zeebx src/audio/wav.rs.
 * Copyright ZeebxTeam contributors, GPL-2.0-or-later.
 * C adaptation and WAV boundary checks by this project's contributors.
 * See third_party/zeebx/LICENSE and third_party/zeebx/NOTICE.md.
 */
#ifndef CNK_ZEEBX_IMA_H
#define CNK_ZEEBX_IMA_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
 uint8_t *samples; /* malloc-allocated interleaved little-endian PCM16 */
 size_t pcm_bytes;
 uint32_t frame_count,sample_rate;
 uint16_t channels;
} cnk_zeebx_ima_pcm;
/* Accepts strictly bounded WAV format tag 17, mono or stereo.
 * Returns 0 and gives ownership of samples to caller, or error (<0).
 * Never returns uninitialized/partial output on failure. No ROM needed. */
int cnk_zeebx_ima_decode_wav(const uint8_t *wav,size_t length,
                             cnk_zeebx_ima_pcm *out);
#endif
