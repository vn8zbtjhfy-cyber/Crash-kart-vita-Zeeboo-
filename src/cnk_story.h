#ifndef CNK_STORY_H
#define CNK_STORY_H
#include "cnk_game.h"
#include <stdint.h>

/* DEVELOPMENT story progression. Source-verified tracks, EXPLICIT developer
 * objectives: not recovered Zeebo chapters, plot, bosses or unlock criteria.
 * Races must finish legitimately; a failed objective unlocks nothing. */
#define CNK_STORY_MAX_STAGES 12u
typedef struct {
 uint32_t resource_id;
 uint32_t race_ordinal;
 uint32_t laps;
 uint32_t best_required_rank;
} cnk_story_stage;
typedef struct {
 uint32_t active;
 uint32_t stage_count;
 uint32_t completed;
 uint32_t racer_count;
 uint32_t player_slot;
 cnk_story_stage stages[CNK_STORY_MAX_STAGES];
 uint8_t achieved_ranks[CNK_STORY_MAX_STAGES];
} cnk_story;

/* Atomic on failure: no guessed track order or objective is inserted. */
int cnk_story_init(cnk_story *out,const cnk_track_catalog *catalog,
                   const cnk_story_stage *stages,uint32_t count,
                   uint32_t racer_count,uint32_t player_slot);
/* 1 when a stage is available, 0 when complete, negative on invalid state. */
int cnk_story_next(const cnk_story *story,cnk_story_stage *out);
/* 1 on success/unlock, 0 when player did not meet the objective (retry),
 * negative on invalid/incomplete/wrong-scene or inconsistent results.
 * Never changes progress on unsuccessful or invalid attempts. */
int cnk_story_complete(cnk_story *story,uint32_t source_id,
                       const cnk_game_classification *board);
#endif
