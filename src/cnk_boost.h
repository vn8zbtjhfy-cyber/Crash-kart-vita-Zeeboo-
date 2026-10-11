#ifndef CNK_BOOST_H
#define CNK_BOOST_H
#include "cnk_drive.h"
#include <stdint.h>
/* Provisional native drift/release boost. Functional mechanics, but NOT
 * asserted to match Zeebo's inputs, timings, physics or visual presentation.
 * No emulator; baseline driving is unchanged until explicitly engaged. */
typedef struct {
 float min_speed, min_steer;
 float charge_seconds, boost_seconds;
 float cooldown_seconds;
 float bonus_speed, bonus_acceleration;
 float drift_turn_factor;
} cnk_boost_tuning;
typedef struct {
 float charge_seconds, boost_left_seconds, cooldown_left_seconds;
 uint32_t boosts_earned;
 int drifting;
} cnk_boost_state;
enum {
 CNK_BOOST_DRIFT_STARTED=1u<<0,
 CNK_BOOST_DRIFT_STOPPED=1u<<1,
 CNK_BOOST_ACTIVATED=1u<<2,
 CNK_BOOST_EXPIRED=1u<<3,
 CNK_BOOST_LAUNCH=1u<<4
};
void cnk_boost_default(cnk_boost_tuning *out);
int cnk_boost_validate(const cnk_boost_tuning *t);
/* One 60 Hz simulation step. Applies to a caller-owned state only on success.
 * Output tuning is ALWAYS copied from base, so no lasting alterations to
 * engine physics after drift/boost. Called only when a race is active. */
int cnk_boost_step(cnk_boost_state *state,const cnk_boost_tuning *config,
   const cnk_drive_tuning *base,int held,float steer,float speed,float dt,
   cnk_drive_tuning *out,uint32_t *events);
#endif
