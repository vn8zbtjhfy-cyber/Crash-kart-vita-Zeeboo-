#ifndef CNK_START_SEQUENCE_H
#define CNK_START_SEQUENCE_H
#include <stdint.h>

/* Portable, deterministic pre-race countdown. The durations below are a
 * temporary native UI policy: Zeebo's exact start timing is NOT verified.
 * Only the GO transition releases the existing 60 Hz race simulation. */
#define CNK_START_LIGHT_US 1000000u
#define CNK_START_GO_VISIBLE_US 750000u
#define CNK_START_EVENT_3  (1u << 0)
#define CNK_START_EVENT_2  (1u << 1)
#define CNK_START_EVENT_1  (1u << 2)
#define CNK_START_EVENT_GO (1u << 3)

typedef enum {
 CNK_START_IDLE=0,
 CNK_START_THREE=3,
 CNK_START_TWO=2,
 CNK_START_ONE=1,
 CNK_START_GO=4,
 CNK_START_RUNNING=5
} cnk_start_phase;

typedef struct {
 cnk_start_phase phase;
 uint32_t remaining_us; /* time to next phase; 0 after GO disappears */
} cnk_start_sequence;

/* Reset for a new race. FIRST 3 event belongs to the start transition. */
void cnk_start_sequence_reset(cnk_start_sequence *s);
/* Advance by dt_us. Returns a subset of dt_us during which kart movement,
 * checkpoints and the race clock MAY advance; *events has each crossed
 * transition exactly once. No race time can leak in before GO. */
int cnk_start_sequence_step(cnk_start_sequence *s,uint32_t dt_us,
                            uint32_t *race_dt_us,uint32_t *events);
int cnk_start_sequence_racing(const cnk_start_sequence *s);
#endif
