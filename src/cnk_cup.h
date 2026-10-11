#ifndef CNK_CUP_H
#define CNK_CUP_H
#include "cnk_game.h"
#include <stdint.h>

/* Opt-in DEVELOPMENT championship engine. Only source-verified circuit IDs
 * can be supplied; neither Zeebo's cup roster nor scoring table is known.
 * No races or points are silently fabricated. */
#define CNK_CUP_MAX_ROUNDS 12u
typedef struct {
 uint32_t active;
 uint32_t racer_count;
 uint32_t round_count;
 uint32_t completed_rounds;
 uint32_t race_ordinals[CNK_CUP_MAX_ROUNDS];
 uint32_t resource_ids[CNK_CUP_MAX_ROUNDS];
 uint32_t points_by_rank[CNK_MAX_RACERS];
 uint32_t total_points[CNK_MAX_RACERS];
 uint32_t rank_sum[CNK_MAX_RACERS];
 uint32_t last_ranks[CNK_MAX_RACERS];
 /* Verified per-round finish positions for reconstructible checkpoint saves. */
 uint8_t round_ranks[CNK_CUP_MAX_ROUNDS][CNK_MAX_RACERS];
} cnk_cup;
typedef struct {
 uint32_t slot, rank, points, rank_sum, last_rank;
} cnk_cup_row;
typedef struct {
 uint32_t count, completed_rounds, total_rounds;
 int finished;
 cnk_cup_row rows[CNK_MAX_RACERS];
} cnk_cup_standings;

/* Caller supplies 2..12 UNIQUE authentic source resource IDs and a
 * strictly descending non-zero points scale, each of length racer_count.
 * Atomic on errors: *out is unchanged. Original cup configuration pending. */
int cnk_cup_init(cnk_cup *out,const cnk_track_catalog *catalog,
                 const uint32_t *resource_ids,uint32_t round_count,
                 const uint32_t *points,uint32_t racer_count);
/* Only after every racer has finished, and only on the current source track.
 * Does not mutate on incomplete, duplicate, malformed or overflowing data. */
int cnk_cup_commit(cnk_cup *cup,uint32_t resource_id,
                   const cnk_game_classification *results);
/* Return 1 for another race, 0 when complete, negative on error. */
int cnk_cup_next_race(const cnk_cup *cup,uint32_t *race_ordinal);
int cnk_cup_standings_read(const cnk_cup *cup,cnk_cup_standings *out);
#endif
