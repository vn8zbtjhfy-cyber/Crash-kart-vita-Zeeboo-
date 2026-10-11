#ifndef CNK_LAUNCH_H
#define CNK_LAUNCH_H
#include "cnk_start_sequence.h"
#include <stdint.h>
/* Optional/provisional launch technique, NOT recovered Zeebo timing.
 * Unlike ordinary throttle, it requires a fresh press INSIDE the final ONE
 * phase, sustained for a minimum interval. This prevents holding throttle
 * since THREE from granting a free boost. Each start has at most one award. */
#define CNK_LAUNCH_HOLD_US 250000u
typedef struct {
 uint32_t charged_us;
 int previous_high;
 int pressed_in_one;
 int consumed;
} cnk_launch_state;
void cnk_launch_reset(cnk_launch_state *state);
/* phase/remaining represent the countdown BEFORE cnk_start_sequence_step;
 * events come from that step. Returns 1 for a valid one-shot award, 0 for
 * no award, negative on invalid arguments. Failure leaves state untouched. */
int cnk_launch_step(cnk_launch_state *state,cnk_start_phase old_phase,
                    uint32_t old_remaining_us,uint32_t dt_us,
                    float throttle,uint32_t start_events);
#endif
