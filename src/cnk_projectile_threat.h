#ifndef CNK_PROJECTILE_THREAT_H
#define CNK_PROJECTILE_THREAT_H
#include "cnk_items.h"
/* Conservative short-horizon alert for the provisional native projectiles.
 * Predicts 2D closest approach from current velocities. This is NOT a
 * recovered Zeebo AI/HUD behaviour or a guarantee of collision. */
typedef struct {
    int incoming;
    cnk_weapon kind;
    uint32_t projectile_slot;
    float eta_seconds;
    float lateral_side; /* signed rightward displacement from kart at read time */
    float closest_distance;
} cnk_projectile_threat;
/* Atomic read: invalid live projectiles/kart leave *out unchanged. */
int cnk_projectile_threat_read(const cnk_item_world *world,uint32_t racer,
                               const cnk_kart *kart,cnk_projectile_threat *out);
#endif
