#ifndef CNK_BOOST_PADS_H
#define CNK_BOOST_PADS_H
#include "cnk_items.h"
#include <stdint.h>
/* Provisional physical speed strips. Positions must be explicitly reviewed
 * against the original source scene; this engine NEVER fabricates placements.
 * Boost strength/time are not verified Zeebo coefficients. */
#define CNK_PAD_MAX 24
#define CNK_PAD_RACERS CNK_ITEM_MAX_RACERS
#define CNK_PAD_EVENT_APPLIED (1u<<0)
typedef struct {
 cnk_vec3 position;
 float radius;
 float cooldown[CNK_PAD_RACERS];
 uint8_t inside[CNK_PAD_RACERS];
} cnk_boost_pad;
typedef struct {
 cnk_boost_pad pads[CNK_PAD_MAX];
 uint32_t count;
} cnk_boost_pad_world;
/* Reject invalid placements atomically. */
int cnk_boost_pad_place(cnk_boost_pad_world *world,cnk_vec3 position,float radius);
/* Per-racer entry-only trigger; increases an existing item's turbo duration
 * rather than double-stacking accelerations. Called only post-GO by session.
 * Successful steps commit everything together; an error changes nothing. */
int cnk_boost_pad_step(cnk_boost_pad_world *world,cnk_item_world *items,
       const cnk_vec3 before[CNK_PAD_RACERS],
       const cnk_kart karts[CNK_PAD_RACERS],
       const uint8_t moving[CNK_PAD_RACERS],uint32_t racers,float dt,
       uint32_t events[CNK_PAD_RACERS]);
#endif
