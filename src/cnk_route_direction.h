#ifndef CNK_ROUTE_DIRECTION_H
#define CNK_ROUTE_DIRECTION_H
#include "cnk_nav.h"
/* Provisional wrong-way warning driven by movement against original DRV0.
 * Not a recovered Zeebo rule. It never modifies physics, lap counters or AI.
 * Time hysteresis rejects spins, single-frame jolts and sideways drifting. */
typedef struct {
 float reverse_seconds;
 float forward_seconds;
 int wrong_way;
} cnk_route_direction;
/* Operates on actual successive kart positions, NOT heading alone.
 * Both projections must be near the source route; distant/off-route,
 * implausible jumps, stationary and lateral motion are ignored.
 * Return 0 on success, <0 for invalid arguments (state left unchanged). */
int cnk_route_direction_step(cnk_route_direction *state,const cnk_sline *route,
                            cnk_vec3 before,cnk_vec3 after,float dt);
#endif
