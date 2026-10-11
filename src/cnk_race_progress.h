#ifndef CNK_RACE_PROGRESS_H
#define CNK_RACE_PROGRESS_H
#include "cnk_race.h"
/* Race ordering derived from source LAP* checkpoints, not just the nearest
 * position on DRV0. No checkpoint credit is granted by this estimator.
 * Route projection is advisory only WITHIN the last confirmed sector.
 * This is provisional ranking policy, not original Zeebo scoring code. */
typedef struct {
 uint32_t passed_gates; /* accepted gates since the most recent finish */
 float sector_fraction; /* [0,1], only if projection belongs to this sector */
} cnk_race_progress;
/* Read-only, 0 on success; rejects inconsistent or non-finite input.
 * A missed checkpoint cannot be counted merely by projecting past it. */
int cnk_race_progress_read(const cnk_race *race,float route_fraction,
                           cnk_race_progress *out);
#endif
