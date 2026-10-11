#ifndef CNK_FRONTEND_H
#define CNK_FRONTEND_H
#include "cnk_game.h"
#include "cnk_start_sequence.h"
#include "cnk_launch.h"
#include "cnk_cup.h"
#include "cnk_cup_manifest.h"
#include "cnk_story.h"
#include "cnk_story_manifest.h"
#include <stdint.h>

/* Portable orchestration for a native game application. It is NOT the
 * original Zeebo menu implementation: original interface, race modes,
 * physics, art, and buttons still require source-grounded reconstruction. */
typedef enum {
 CNK_FRONTEND_EMPTY=0,
 CNK_FRONTEND_SELECT=1,
 CNK_FRONTEND_RACE=2,
 CNK_FRONTEND_PAUSE=3,
 CNK_FRONTEND_RESULTS=4
} cnk_frontend_state;

typedef struct {
 cnk_track_catalog catalog;     /* source-backed and immutable after init */
 cnk_game game;                 /* fully owned; source VFS is borrowed */
 cnk_frontend_state state;
 uint32_t selected_race;       /* ordinal of verified race, not FUFS index */
 uint32_t racer_count;
 uint32_t total_laps;
 uint32_t starts;
 int practice_mode; /* opt-in native SOLO trial, not verified Zeebo mode */
 cnk_cup cup; /* optional authenticated-track multi-race tournament */
 cnk_story story; /* opt-in development sequence, NOT original Zeebo story */
 cnk_story_manifest story_manifest;
 int story_manifest_ready;
 int story_last_attempt_success;
 cnk_cup_manifest cup_manifest; /* optional developer manifest, no default */
 int cup_manifest_ready;
 cnk_start_sequence start_sequence; /* motion blocked until GO */
 cnk_launch_state launch; /* optional one-shot acceleration from timed start */
 uint32_t start_events; /* drained once by app event adapter */
} cnk_frontend;

/* Returns 0 or a negative code. Call on a zero-initialized object. */
int cnk_frontend_init(cnk_frontend *ui,const fufs_file *vfs,
                      uint32_t racer_count,uint32_t total_laps);
void cnk_frontend_close(cnk_frontend *ui);
/* Selection wraps in original VFS order. Available only in SELECT state. */
int cnk_frontend_select(cnk_frontend *ui,int direction);
const cnk_track_entry *cnk_frontend_selected(const cnk_frontend *ui);
/* Selection-screen-only provisional solo training; unchanged racer count is
 * restored for normal races without reconstructing prior game systems. */
int cnk_frontend_toggle_practice(cnk_frontend *ui);
/* Native development option: cycle 1,2,3,5 laps from SELECT. Not Zeebo menu. */
int cnk_frontend_cycle_laps(cnk_frontend *ui);
/* Loads scene/geometry/camera/karts from original data.vfs, using STRT. */
int cnk_frontend_start(cnk_frontend *ui);
/* Optional provisional Cup: source-backed track IDs, explicit scoring.
 * Not an assertion of authentic Zeebo round grouping or scoring. */
int cnk_frontend_cup_begin(cnk_frontend *ui,const uint32_t *resource_ids,
                           uint32_t count,const uint32_t *points);
/* On a committed result, move to the next source-backed race. Returns 1
 * if all Cup rounds are over; 0 after successfully loading another race. */
int cnk_frontend_cup_next(cnk_frontend *ui);
/* Development story: explicit source-backed objectives, no fabricated plot. */
int cnk_frontend_story_begin(cnk_frontend *ui,const cnk_story_stage *stages,uint32_t count);
int cnk_frontend_story_configure(cnk_frontend *ui,const cnk_story_manifest *config);
/* 1 if story already finished, 0 after next stage loaded, negative on error.
 * Failed objectives cannot unlock the next stage: RESTART retries. */
int cnk_frontend_story_next(cnk_frontend *ui);
/* Resume ONLY previously validated completed stages of this developer manifest. */
int cnk_frontend_story_resume(cnk_frontend *ui,const cnk_story *verified);
/* Restores only a verified checkpoint after >=1 completed round; loads the
 * next original source track. No unfinished-round progress is fabricated. */
int cnk_frontend_cup_resume(cnk_frontend *ui,const cnk_cup *verified);
/* SELECT-only, developer-provided authenticated cup. Atomic on error. */
int cnk_frontend_cup_configure(cnk_frontend *ui,const cnk_cup_manifest *config);
/* Only RACE/PAUSE are allowed; pause freezes clock and movement. */
int cnk_frontend_toggle_pause(cnk_frontend *ui);
/* Deterministic 60 Hz physics step; no gameplay advances in menu/pause. */
int cnk_frontend_update(cnk_frontend *ui,cnk_driver_input input,uint32_t dt_us);
/* Refresh result state without stepping, useful after late finish event. */
int cnk_frontend_refresh(cnk_frontend *ui);
/* The UI/audiovisual adapter drains countdown signals once per frame. */
uint32_t cnk_frontend_take_start_events(cnk_frontend *ui);
/* Reload selected original track with same lap and competitor settings. */
int cnk_frontend_restart(cnk_frontend *ui);
/* Releases scene resources and returns to verified race selector. */
int cnk_frontend_back_to_select(cnk_frontend *ui);
#endif
