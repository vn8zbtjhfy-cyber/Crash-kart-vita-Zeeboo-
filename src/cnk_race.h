#ifndef CNK_RACE_H
#define CNK_RACE_H
#include "cnk_scene.h"
#include <stdint.h>
/* Chronometer/checkpoints derived exclusively from the original Zeebo
 * SCEN DRV0 and finite LAP* segments. This is NOT the original Polarbit
 * lap-scoring code; its runtime semantics still require reference testing.
 * A track is accepted only when every checkpoint crosses the source driving
 * polyline unambiguously once. Special/ambiguous scenes fail explicitly.
 */
#define CNK_MAX_GATES 32
#define CNK_RACE_GATE 1u
#define CNK_RACE_LAP 2u
#define CNK_RACE_FINISH 4u

typedef struct {
  char id[5];
  cnk_vec3 a,b;
  float fraction; /* intersection along DRV0, normalized [0..1) */
  int direction; /* original path's crossing sign, +1 or -1 */
} cnk_race_gate;
typedef struct {
  uint32_t gate_count;
  cnk_race_gate gates[CNK_MAX_GATES]; /* first gate = LAP0 (finish) */
} cnk_race_track;
typedef struct {
  const cnk_race_track *track; /* borrowed */
  uint32_t total_laps, completed_laps;
  uint32_t next_gate; /* index into cyclic gates: initially 1 */
  uint32_t last_accepted_gate; /* UINT32_MAX until a verified crossing */
  uint64_t start_ms, lap_start_ms, last_ms, best_lap_ms, last_lap_ms;
  uint64_t finish_ms; /* interpolated sub-frame finish, valid when finished */
  /* Provisional source-gate sector timing, not original Zeebo splits. */
  uint64_t sector_start_ms, last_sector_ms;
  uint64_t best_sector_ms[CNK_MAX_GATES];
  uint32_t last_sector_gate; /* UINT32_MAX until a validated split */
  int sector_valid; /* invalid shortcut must not produce record splits */
  signed char last_side[CNK_MAX_GATES]; /* remembers sign at on-line vertices */
  int running, finished;
} cnk_race;

/* -3 = no ordinary route, -4 = ambiguous geometry/extra LAPs. */
int cnk_race_track_build(const cnk_scene *s,cnk_race_track *out);
/* Lap count is an explicit caller option because this property cannot yet be
 * recovered reliably from SCEN. Clock supplied by host in monotonic ms. */
int cnk_race_start(cnk_race *race,const cnk_race_track *track,
                   uint32_t laps,uint64_t now_ms);
/* One sequential kart motion step. Returns bit flags CNK_RACE_* or 0;
 * negative numbers are errors. No physics/AI is simulated here. */
int cnk_race_advance(cnk_race *race,cnk_vec3 before,cnk_vec3 after,uint64_t now_ms);
#endif
