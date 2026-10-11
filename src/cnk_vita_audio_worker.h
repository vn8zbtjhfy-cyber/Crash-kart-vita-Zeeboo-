#ifndef CNK_VITA_AUDIO_WORKER_H
#define CNK_VITA_AUDIO_WORKER_H
#include "cnk_vita_audio.h"
#include <psp2/types.h>
/* A dedicated SceAudioOut thread, independent from display vblank.
 * Access to all mixer voices and owned cue data must be guarded with lock().
 * No original game sound mapping is implied by this adapter.
 * Hardware behavior still requires tests on a real PS Vita. */
typedef struct {
 cnk_vita_audio output;
 SceUID mutex;
 SceUID thread;
 int running;
 int stop_requested; /* read/written ONLY with mutex held */
 int thread_error;   /* read only after wait, or with mutex held */
} cnk_vita_audio_worker;
int cnk_vita_audio_worker_open(cnk_vita_audio_worker *a);
int cnk_vita_audio_worker_start(cnk_vita_audio_worker *a);
int cnk_vita_audio_worker_lock(cnk_vita_audio_worker *a);
int cnk_vita_audio_worker_unlock(cnk_vita_audio_worker *a);
/* Thread output errors; 0 means no error observed yet. */
int cnk_vita_audio_worker_status(cnk_vita_audio_worker *a);
/* Joins the audio thread BEFORE releasing the output port or cue data.
 * Caller closes cnk_sound_cues AFTER this function returns. */
void cnk_vita_audio_worker_close(cnk_vita_audio_worker *a);
#endif
