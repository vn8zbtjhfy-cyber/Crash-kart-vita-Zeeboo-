#include "cnk_boost.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void){
 cnk_boost_tuning cfg={0};cnk_drive_tuning base={0},out={0};
 cnk_boost_state state={0};uint32_t ev=0;
 cnk_boost_default(&cfg);cnk_drive_default_tuning(&base);
 assert(!cnk_boost_validate(&cfg));
 assert(cnk_boost_step(&state,&cfg,&base,0,0.8f,20,1.0f/60,&out,&ev)==0);
 assert(!ev&&!state.drifting&&out.max_speed==base.max_speed);
 /* At low speed or with no steering there can be no charged drift. */
 for(int i=0;i<100;i++){
  assert(!cnk_boost_step(&state,&cfg,&base,1,0.8f,3,1.0f/60,&out,&ev));
  assert(!state.drifting&&!ev);
 }
 assert(!cnk_boost_step(&state,&cfg,&base,1,0.8f,18,1.0f/60,&out,&ev));
 assert(state.drifting&&(ev&CNK_BOOST_DRIFT_STARTED));
 assert(out.turn_rate>base.turn_rate);
 for(int i=0;i<10;i++)assert(!cnk_boost_step(&state,&cfg,&base,1,0.8f,18,1.0f/60,&out,&ev));
 assert(!cnk_boost_step(&state,&cfg,&base,0,0.8f,18,1.0f/60,&out,&ev));
 assert((ev&CNK_BOOST_DRIFT_STOPPED)&&!(ev&CNK_BOOST_ACTIVATED));
 assert(!state.boosts_earned&&!state.boost_left_seconds);
 for(int i=0;i<70;i++)assert(!cnk_boost_step(&state,&cfg,&base,1,0.8f,22,1.0f/60,&out,&ev));
 assert(state.drifting);
 assert(fabsf(state.charge_seconds-cfg.charge_seconds)<0.0001f);
 assert(!cnk_boost_step(&state,&cfg,&base,0,0.8f,22,1.0f/60,&out,&ev));
 assert((ev&CNK_BOOST_ACTIVATED)&&(ev&CNK_BOOST_DRIFT_STOPPED));
 assert(state.boosts_earned==1 && state.boost_left_seconds>0);
 assert(out.max_speed>base.max_speed&&out.acceleration>base.acceleration);
 /* Holding the drift control through an active boost cannot farm more. */
 for(int i=0;i<15;i++){
  assert(!cnk_boost_step(&state,&cfg,&base,1,0.8f,25,1.0f/60,&out,&ev));
  assert(!state.drifting && state.boosts_earned==1);
 }
 int expired=0;
 for(int i=0;i<120;i++){
  assert(!cnk_boost_step(&state,&cfg,&base,0,0.8f,25,1.0f/60,&out,&ev));
  if(ev&CNK_BOOST_EXPIRED)expired++;
 }
 assert(expired==1&&state.boost_left_seconds==0);
 assert(out.max_speed==base.max_speed);
 /* Input errors never corrupt the previous state or the caller's tuning. */
 cnk_boost_state unchanged=state;
 assert(cnk_boost_step(&state,&cfg,&base,1,0.9f,NAN,1.0f/60,&out,&ev)<0);
 assert(memcmp(&state,&unchanged,sizeof(state))==0);
 cfg.bonus_speed=NAN;
 assert(cnk_boost_step(&state,&cfg,&base,1,0.9f,20,1.0f/60,&out,&ev)<0);
 assert(memcmp(&state,&unchanged,sizeof(state))==0);
 puts("PASS: real deterministic boost code: minimum speed/steer, short tap, charge cap, release, active boost, expiration, invalid input");
 return 0;
}
