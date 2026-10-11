#ifndef CNK_AI_TACTICS_H
#define CNK_AI_TACTICS_H
#include "cnk_items.h"
#include "cnk_nav.h"
/* Experimental obstacle/pickup decisions layered over the source DRV0 route
 * controller, NOT the original Zeebo CPU strategy. Only reacts to explicit
 * runtime obstacles and authored pickups; does not fabricate track content. */
#define CNK_AI_TACTIC_AVOID_HAZARD 1u
#define CNK_AI_TACTIC_SEEK_PICKUP 2u
#define CNK_AI_TACTIC_AVOID_PROJECTILE 4u
/* Receives an already-computed source-route control decision. On failure the
 * input and flags remain untouched. When nothing relevant is present they are
 * returned bit-for-bit unchanged. Applies only to AI slots (not player). */
int cnk_ai_tactics_apply(const cnk_item_world *items,
     const cnk_sline *route,uint32_t slot,const cnk_kart *kart,
     cnk_driver_input *controls,uint32_t *flags);
#endif
