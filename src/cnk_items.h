#ifndef CNK_ITEMS_H
#define CNK_ITEMS_H
#include "cnk_drive.h"
#include <stdint.h>
/* Item foundation for the native game. Weapon names are supported by the
 * source executable paths; mechanics below are PROVISIONAL, not recovered
 * Zeebo code. No pickups are spawned automatically at fabricated positions. */
#define CNK_ITEM_MAX_RACERS 6
#define CNK_ITEM_MAX_PICKUPS 32
#define CNK_ITEM_MAX_BOMBS 16
#define CNK_ITEM_MAX_MISSILES 12
#define CNK_ITEM_MAX_HAZARDS 16

typedef enum {CNK_PICKUP_WUMPA=1,CNK_PICKUP_CRATE=2} cnk_pickup_kind;
typedef enum {CNK_WEAPON_NONE=0,CNK_WEAPON_TURBO=1,CNK_WEAPON_BOMB=2,CNK_WEAPON_MISSILE=3,
 CNK_WEAPON_TNT=4,CNK_WEAPON_OIL=5,CNK_WEAPON_SHIELD=6,
 CNK_WEAPON_TROPY_CLOCK=7,CNK_WEAPON_CURSED_CANDLE=8,
 CNK_WEAPON_AKU_AKU=9,CNK_WEAPON_KART_JACKING=10} cnk_weapon;
enum {
 CNK_ITEM_EVENT_WUMPA=1u<<0,
 CNK_ITEM_EVENT_WEAPON=1u<<1,
 CNK_ITEM_EVENT_USED=1u<<2,
 CNK_ITEM_EVENT_TURBO_ON=1u<<3,
 CNK_ITEM_EVENT_TURBO_OFF=1u<<4,
 CNK_ITEM_EVENT_BOMB_FIRED=1u<<5,
 CNK_ITEM_EVENT_HIT=1u<<6,
 CNK_ITEM_EVENT_MISSILE_FIRED=1u<<7,
 CNK_ITEM_EVENT_HAZARD_DROPPED=1u<<8,
 CNK_ITEM_EVENT_OIL_HIT=1u<<9,
 CNK_ITEM_EVENT_TNT_HIT=1u<<10,
 CNK_ITEM_EVENT_MISSILE_HIT=1u<<11,
 CNK_ITEM_EVENT_BOMB_WALL=1u<<12, /* optional source-geometry contact */
 CNK_ITEM_EVENT_MISSILE_WALL=1u<<13,
 CNK_ITEM_EVENT_SHIELD_ON=1u<<15,
 CNK_ITEM_EVENT_SHIELD_OFF=1u<<16,
 CNK_ITEM_EVENT_SHIELD_BLOCK=1u<<17,
 CNK_ITEM_EVENT_WUMPA_LOST=1u<<18,
 CNK_ITEM_EVENT_CLOCK_ON=1u<<19,
 CNK_ITEM_EVENT_CLOCK_OFF=1u<<20,
 CNK_ITEM_EVENT_CANDLE_ON=1u<<21,
 CNK_ITEM_EVENT_CANDLE_OFF=1u<<22,
 CNK_ITEM_EVENT_AKU_ON=1u<<23,
 CNK_ITEM_EVENT_AKU_OFF=1u<<24,
 CNK_ITEM_EVENT_AKU_TACKLE=1u<<25,
 CNK_ITEM_EVENT_KART_JACK=1u<<26,
 CNK_ITEM_EVENT_PAD_BOOST=1u<<14
};
typedef struct {
 cnk_vec3 position;
 float radius;
 float unavailable_seconds;
 cnk_pickup_kind kind;
 cnk_weapon reward; /* required for crates: never invent random drops */
} cnk_item_pickup;
typedef struct {
 cnk_vec3 position;
 float heading;
 float lifetime;
 uint32_t owner;
 int active;
} cnk_item_bomb;
/* Experimental projectile physics: the original Zeebo parameters, animation,
 * collision volumes and target selection are NOT recovered. */
typedef struct {
 cnk_vec3 position;
 float heading, lifetime;
 uint32_t owner, target; /* target==UINT32_MAX until an eligible rival is detected */
 int active;
} cnk_item_missile;
typedef struct {
 cnk_vec3 position;
 float lifetime;
 uint32_t owner;
 cnk_weapon kind; /* TNT or OIL */
 int active;
} cnk_item_hazard;
typedef struct {
 uint8_t wumpa;
 cnk_weapon held;
 float turbo_seconds;
 float stun_seconds;
 float oil_seconds; /* temporary reduced steering/traction */
 float shield_seconds; /* provisional timed invulnerability; source LANG confirms shield exists */
 float aku_seconds; /* provisional Aku-Aku speed and tackle, confirmed in source LANG */
 float aku_tackle_cooldown; /* prevent repeated impact every 60Hz step */
} cnk_item_racer;
typedef struct {
 cnk_item_racer racers[CNK_ITEM_MAX_RACERS];
 cnk_item_pickup pickups[CNK_ITEM_MAX_PICKUPS];
 cnk_item_bomb bombs[CNK_ITEM_MAX_BOMBS];
 cnk_item_missile missiles[CNK_ITEM_MAX_MISSILES];
 cnk_item_hazard hazards[CNK_ITEM_MAX_HAZARDS];
 uint32_t pickup_count;
 /* One global effect of each type. The 3s/4s values are provisional.
  * The source LANG confirms the mechanics, NOT exact duration/strength.
  * All ticks are fixed-step simulation seconds, never wall clock. */
 float clock_seconds, candle_seconds;
 uint32_t clock_owner, candle_owner;
} cnk_item_world;
/* Author-reviewed placements only. Invalid inputs never mutate the world. */
int cnk_items_place(cnk_item_world *world,cnk_pickup_kind kind,
                    cnk_vec3 pos,float radius,cnk_weapon reward);
/* Direct inventory use at race time. Returns 1 if consumed, 0 if none,
 * <0 for invalid state. Projectiles and traps have fixed-size pools;
 * an exhausted pool NEVER consumes an equipped weapon. */
int cnk_items_use(cnk_item_world *world,uint32_t owner,const cnk_kart *kart,
                  uint32_t *events);
/* Provisional source-described Kart Jacking. `eligible` must be decided by
 * the caller from SAME LAP + NEXT VERIFIED GATE; this routine only swaps the
 * two kart poses, never race clocks/gates/rounds. Returns target index + 1,
 * zero when no safe target (item retained), or negative on invalid input. */
int cnk_items_try_jack(cnk_item_world *world,uint32_t owner,
                     cnk_kart karts[CNK_ITEM_MAX_RACERS],
                     const uint8_t eligible[CNK_ITEM_MAX_RACERS],
                     uint32_t count,uint32_t events[CNK_ITEM_MAX_RACERS]);
/* Opt-in opponent item decision. Existing route AI remains unmodified until it
 * actually owns a pickup; this is a provisional heuristic, not Zeebo AI. */
int cnk_items_ai_use(cnk_item_world *world,uint32_t owner,
                     const cnk_kart karts[CNK_ITEM_MAX_RACERS],
                     const uint8_t movable[CNK_ITEM_MAX_RACERS],
                     uint32_t count,uint32_t *events);
/* Deterministic 60 Hz tick after kart physics. Swept collision prevents
 * tunnelling past pickups; bomb motion and contact modify kart speed. */
int cnk_items_step(cnk_item_world *world,cnk_kart karts[CNK_ITEM_MAX_RACERS],
                   const cnk_vec3 before[CNK_ITEM_MAX_RACERS],
                   const uint8_t movable[CNK_ITEM_MAX_RACERS],
                   uint32_t count,float dt,
                   uint32_t events[CNK_ITEM_MAX_RACERS]);
/* Opt-in projectile vs. source steep-triangle sweep. A missing/ambiguous
 * Zeebo collision-material classification prevents enabling by default.
 * Legacy item simulation remains identical while the option is disabled. */
int cnk_items_step_world(cnk_item_world *world,
                   cnk_kart karts[CNK_ITEM_MAX_RACERS],
                   const cnk_vec3 before[CNK_ITEM_MAX_RACERS],
                   const uint8_t movable[CNK_ITEM_MAX_RACERS],
                   uint32_t count,float dt,
                   uint32_t events[CNK_ITEM_MAX_RACERS],
                   const cnk_collision_grid *walls,float max_abs_normal_y);
/* Apply only temporary item's impulse; no baseline tuning is ever changed. */
int cnk_items_tune(const cnk_item_world *world,uint32_t racer,
                   const cnk_drive_tuning *base,cnk_drive_tuning *out);
#endif
