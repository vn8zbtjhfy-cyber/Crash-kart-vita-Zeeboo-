#ifndef CNK_SOUND_CUES_H
#define CNK_SOUND_CUES_H
#include "cnk_app_runtime.h"
#include "cnk_audio.h"
#include <stdint.h>

/* Optional, independently reviewed cue -> WAV index assignments. A valid
 * WAV resource ID proves that bytes are FROM the Zeebo ROM, NOT that it is
 * the correct historical sound for the chosen event. No map is bundled. */
#define CNK_SOUND_CUES_MAX 24u
#define CNK_SOUND_CUE_APP 1u
#define CNK_SOUND_CUE_PLAYER 2u

typedef struct {
 uint32_t domain;   /* APP or PLAYER */
 uint32_t event_bit; /* one recognized event */
 uint32_t vfs_index;
 uint32_t resource_id;
 float volume;
 cnk_wav_asset asset; /* owned while router exists */
} cnk_sound_cue;

typedef struct {
 cnk_audio_mixer *mixer; /* borrowed, must outlive this router */
 cnk_sound_cue cues[CNK_SOUND_CUES_MAX];
 unsigned count;
} cnk_sound_cues;

/* Format: APP|PLAYER<TAB>EVENT_NAME<TAB>VFS_INDEX<TAB>RESOURCE_ID_HEX<TAB>VOLUME
 * Strictly atomically verified; missing optional file => -2.
 * Asset identity is validated, but event->sound identity remains user-supplied.
 * Mixer must be quiescent during loading/closing (no concurrent audio thread).
 */
int cnk_sound_cues_read(cnk_sound_cues *router,cnk_audio_mixer *mixer,
                        const fufs_file *vfs,const char *path);
/* Consume one transient runtime snapshot; only the human kart's events fire.
 * On saturation, reclaim a one-shot voice owned by this router, never music;
 * drop incoming SFX if there is no safe slot (without aborting the race).
 * No guessed default sounds. No sound without an explicit valid assignment. */
int cnk_sound_cues_dispatch(cnk_sound_cues *router,
                             const cnk_app_snapshot *snapshot,unsigned *played);
/* Stop every voice referencing an owned cue sample before freeing its bytes. */
void cnk_sound_cues_close(cnk_sound_cues *router);
#endif
