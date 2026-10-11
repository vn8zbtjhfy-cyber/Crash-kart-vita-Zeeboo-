#ifndef CNK_MUSIC_CUES_H
#define CNK_MUSIC_CUES_H
#include "cnk_music.h"
#include "cnk_app_runtime.h"
#include <stddef.h>

/* The decoded tracks are genuine Zeebo audio. Track-to-scene assignments
 * MUST be independently reviewed. No mappings are bundled with the game.
 * A source ID in this map proves which MP3 was decoded, NOT what played
 * at a particular point in the historical game. */
#define CNK_MUSIC_CUES_MAX 32u
#define CNK_MUSIC_DIR_MAX 256u

typedef struct {
    cnk_frontend_state state;
    uint32_t scene_index; /* UINT32_MAX is wildcard; only RACE supports exact */
    unsigned track;       /* slot 0..6 */
    float volume;
    int loop;
} cnk_music_cue;

typedef struct {
    cnk_music player;
    cnk_music_cue cues[CNK_MUSIC_CUES_MAX];
    char directory[CNK_MUSIC_DIR_MAX];
    unsigned count;
    float playing_volume;
    int playing_loop;
    int initialized;
    int race_music_paused; /* pause only when no explicit PAUSE soundtrack */
} cnk_music_cues;

typedef int (*cnk_music_mutex_fn)(void *context);
/* TSV: STATE\tSCENE_INDEX|*\tORIGINAL_MP3_VFS_INDEX\tVOLUME\tLOOP
 * STATE = SELECT|RACE|PAUSE|RESULTS, VOL in (0,1], LOOP=0 or 1.
 * Exact RACE scene takes priority over wildcard. Not a historical mapping.
 * Parsing is atomic: on any invalid row, no mixer state is changed.
 * No file means -2 and no assigned soundtrack. */
int cnk_music_cues_read(cnk_music_cues *cue, cnk_audio_mixer *mixer,
                        const char *tsv_path, const char *wav_directory);
/* Stage WAV I/O without holding the audio mutex, then stop/swap voices
 * atomically while the same mutex used by the worker is held.
 * State with no mapping becomes silent EXCEPT PAUSE: existing race music
 * is frozen at its PCM cursor and resumed without reloading. No track guessed.
 * Call once after a frontend snapshot. */
int cnk_music_cues_dispatch(cnk_music_cues *cue,const cnk_app_snapshot *snapshot,
                            cnk_music_mutex_fn lock, cnk_music_mutex_fn unlock,
                            void *context, unsigned *changed);
/* Worker MUST be stopped/joined before this closes music and frees PCM. */
void cnk_music_cues_close(cnk_music_cues *cue);
#endif
