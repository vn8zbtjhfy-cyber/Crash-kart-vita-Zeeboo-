#include "cnk_boost.h"
#include <math.h>
#include <limits.h>

void cnk_boost_default(cnk_boost_tuning *t){
 if(!t)return;
 /* Provisional, deliberately isolated so Zeebo measurements can replace it. */
 *t=(cnk_boost_tuning){8.0f,0.35f,0.60f,1.00f,0.35f,12.0f,40.0f,1.18f};
}
int cnk_boost_validate(const cnk_boost_tuning *t){
 if(!t)return -1;
 const float v[]={t->min_speed,t->min_steer,t->charge_seconds,
  t->boost_seconds,t->cooldown_seconds,t->bonus_speed,
  t->bonus_acceleration,t->drift_turn_factor};
 for(unsigned i=0;i<sizeof(v)/sizeof(v[0]);i++)
  if(!isfinite(v[i])||v[i]<0.0f||v[i]>1000.0f)return -2;
 if(t->min_steer>1.0f||t->charge_seconds<=0.0f||
    t->boost_seconds<=0.0f||t->drift_turn_factor<=0.0f)return -3;
 return 0;
}
int cnk_boost_step(cnk_boost_state *state,const cnk_boost_tuning *cfg,
  const cnk_drive_tuning *base,int held,float steer,float speed,float dt,
  cnk_drive_tuning *out,uint32_t *events){
 if(!state||!out||!events||cnk_boost_validate(cfg)||cnk_drive_validate(base)||
    !isfinite(steer)||!isfinite(speed)||!isfinite(dt)||dt<=0.0f||dt>0.10f||
    !isfinite(state->charge_seconds)||!isfinite(state->boost_left_seconds)||
    !isfinite(state->cooldown_left_seconds)||state->charge_seconds<0.0f||
    state->boost_left_seconds<0.0f||state->cooldown_left_seconds<0.0f)
  return -1;
 cnk_boost_state n=*state;
 cnk_drive_tuning tuning=*base;
 uint32_t e=0;
 /* Counts down in simulation time, never wall clock; paused/start-gated race
  * performs no step. Only edge release after a sufficiently charged drift
  * produces a boost; holding R forever cannot farm boosts. */
 if(n.boost_left_seconds>0.0f){
  n.boost_left_seconds=fmaxf(0.0f,n.boost_left_seconds-dt);
  if(n.boost_left_seconds<=0.0f)e|=CNK_BOOST_EXPIRED;
 }
 if(n.cooldown_left_seconds>0.0f)
  n.cooldown_left_seconds=fmaxf(0.0f,n.cooldown_left_seconds-dt);
 int eligible=held && fabsf(steer)>=cfg->min_steer &&
  speed>=cfg->min_speed && n.boost_left_seconds<=0.0f &&
  n.cooldown_left_seconds<=0.0f;
 if(eligible){
  if(!n.drifting){e|=CNK_BOOST_DRIFT_STARTED;n.charge_seconds=0.0f;}
  n.drifting=1;
  n.charge_seconds=fminf(cfg->charge_seconds,n.charge_seconds+dt);
  tuning.turn_rate*=cfg->drift_turn_factor;
 }else if(n.drifting){
  e|=CNK_BOOST_DRIFT_STOPPED;
  n.drifting=0;
  if(!held && n.charge_seconds>=cfg->charge_seconds &&
     n.boosts_earned<UINT32_MAX){
   n.boost_left_seconds=cfg->boost_seconds;
   n.cooldown_left_seconds=cfg->cooldown_seconds;
   n.boosts_earned++;
   e|=CNK_BOOST_ACTIVATED;
  }
  n.charge_seconds=0.0f;
 }
 if(n.boost_left_seconds>0.0f){
  tuning.max_speed+=cfg->bonus_speed;
  tuning.acceleration+=cfg->bonus_acceleration;
 }
 if(cnk_drive_validate(&tuning))return -2;
 *out=tuning;*state=n;*events=e;
 return 0;
}
