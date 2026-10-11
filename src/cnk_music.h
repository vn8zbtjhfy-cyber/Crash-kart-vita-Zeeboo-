#ifndef CNK_MUSIC_H
#define CNK_MUSIC_H
#include "cnk_audio.h"
/* Music is decoded offline from the *original* Zeebo MP3 assets into
 * uncompressed RIFF PCM, so PS Vita needs no MP3 emulator or runtime codec.
 * Only one selected track is loaded into RAM, never all seven simultaneously.
 * This component does NOT infer which song belongs to any menu or course.
 */
#define CNK_MUSIC_TRACKS 7u

typedef struct {
    cnk_audio_mixer *mixer; /* borrowed, caller retains and pumps mixer */
    cnk_wav_asset decoded; /* owns the WAV for currently selected track */
    int voice; /* active mixer slot or -1 */
    int track; /* [0..6] or -1 */
} cnk_music;

/* Asset IDs from the original Zeebo data.vfs, in increasing VFS order. */
uint32_t cnk_music_original_index(unsigned track);
/* Initializes without loading any song. */
int cnk_music_init(cnk_music *player,cnk_audio_mixer *mixer);
/* WAV must be PCM stereo/mono, validated with existing RIFF parser.
 * Safe even when switching tracks: old voice stopped before old data freed.
 * Stops/discards previous song on load error. */
int cnk_music_load(cnk_music *player,unsigned track,const char *wav_path);
/* Start the selected track; loops when requested. Set 0<volume<=1. */
int cnk_music_play(cnk_music *player,float volume,int loop);
int cnk_music_stop(cnk_music *player);
/* Stop and release owned music data; mixer must stay alive until this call. */
void cnk_music_close(cnk_music *player);
#endif
