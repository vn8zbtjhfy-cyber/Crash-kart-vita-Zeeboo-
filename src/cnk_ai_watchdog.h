#ifndef CNK_AI_WATCHDOG_H
#define CNK_AI_WATCHDOG_H
#include "cnk_scene.h"
/* Conservative native anti-stall guard for rival karts. Not original Zeebo AI.
 * A rescue is requested ONLY after the kart stays inside a small horizontal
 * area for 12 seconds of ACTIVE race simulation; 12 s is not wall time. */
#define CNK_AI_STUCK_SECONDS 12.0f
#define CNK_AI_STUCK_RADIUS 3.0f
typedef struct {
 cnk_vec3 anchor;
 float still_seconds;
 int initialized;
} cnk_ai_watchdog;
/* Returns 1 for a stuck-racer rescue request, 0 otherwise, <0 if invalid.
 * in_control=0 (finished/stunned) disarms, preventing penalty or false rescue.
 * Requests reset internal timing so recovery cannot happen on every frame. */
int cnk_ai_watchdog_step(cnk_ai_watchdog *watch,cnk_vec3 pos,
                         float dt,int in_control);
#endif
