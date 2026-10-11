#ifndef CNK_AI_TRAFFIC_H
#define CNK_AI_TRAFFIC_H
#include "cnk_drive.h"
#include <stdint.h>
/* Provisional traffic awareness for CPU-controlled rivals. Derived from live
 * racer positions, NOT recovered Zeebo opponent tactics or track lane data. */
#define CNK_AI_TRAFFIC_FOLLOW 8u
#define CNK_AI_TRAFFIC_PASS 16u
/* Only reads the original DRV0 in overtaking mode. This routine never affects
 * the human kart or awards checkpoint, items or timing results. `flags` is
 * unchanged on invalid arguments. High-priority weapon evasions should be
 * applied first, then call only when no weapon evasion was requested. */
int cnk_ai_traffic_apply(const cnk_sline *route,
 const cnk_kart karts[],const uint8_t movable[],uint32_t count,uint32_t slot,
 cnk_driver_input *control,uint32_t *flags);
#endif
