#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

static void tick(cnk_app_runtime *app,uint64_t *clock,uint32_t buttons,
                 cnk_driver_input in,cnk_app_snapshot *snap){
 *clock+=16667u;
 assert(!cnk_app_runtime_tick(app,*clock,buttons,in,snap));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,1,3));
 uint64_t clock=1000000;
 cnk_driver_input driving={1,0,0.8f};
 assert(!cnk_app_runtime_tick(&app,clock,0,driving,&snap));
 tick(&app,&clock,CNK_UI_CONFIRM,driving,&snap);
 assert(snap.state==CNK_FRONTEND_RACE);
 /* Holding R before GO must not move the kart or charge any meter. */
 for(int i=0;i<180;i++){
  tick(&app,&clock,CNK_UI_DRIFT,driving,&snap);
  assert(snap.hud.elapsed_ms==0);
  assert(snap.player_boost.charge_seconds==0);
  assert(!snap.boost_active);
 }
 int started=0;
 for(int i=0;i<80;i++){
  tick(&app,&clock,CNK_UI_DRIFT,driving,&snap);
  if(snap.app_events&CNK_APP_EVENT_DRIFT_START)started++;
 }
 assert(started==1 && snap.player_boost.drifting);
 assert(snap.boost_charge_fraction>0.99f);
 tick(&app,&clock,0,driving,&snap);
 assert(snap.app_events&CNK_APP_EVENT_BOOST_START);
 assert(snap.boost_active);
 assert(snap.player_boost.boosts_earned==1);
 float speed_start=app.frontend.game.session.racers[0].kart.speed;
 float peak_speed=speed_start;
 for(int i=0;i<30;i++){
  tick(&app,&clock,0,driving,&snap);
  float v=app.frontend.game.session.racers[0].kart.speed;
  if(v>peak_speed)peak_speed=v;
 }
 assert(peak_speed>app.frontend.game.session.tuning.max_speed);
 assert(peak_speed>speed_start);
 assert(snap.boost_active);
 const cnk_boost_state held=snap.player_boost;
 uint64_t frames=app.frontend.game.simulation_frames;
 tick(&app,&clock,CNK_UI_PAUSE,driving,&snap);
 assert(snap.state==CNK_FRONTEND_PAUSE);
 for(int i=0;i<15;i++){
  tick(&app,&clock,CNK_UI_DRIFT,driving,&snap);
  assert(app.frontend.game.simulation_frames==frames);
  assert(snap.player_boost.boost_left_seconds==held.boost_left_seconds);
 }
 tick(&app,&clock,CNK_UI_PAUSE,driving,&snap);
 assert(snap.state==CNK_FRONTEND_RACE);
 int expired=0;
 for(int i=0;i<100;i++){
  tick(&app,&clock,0,driving,&snap);
  if(snap.app_events&CNK_APP_EVENT_BOOST_END)expired++;
 }
 assert(expired==1 && !snap.boost_active);
 /* Reset completely clears earned boosts, without rebuilding the VFS. */
 tick(&app,&clock,CNK_UI_PAUSE,driving,&snap);
 assert(snap.state==CNK_FRONTEND_PAUSE);
 tick(&app,&clock,0,driving,&snap);
 tick(&app,&clock,CNK_UI_RESTART,driving,&snap);
 assert(snap.state==CNK_FRONTEND_RACE&&snap.start_phase==CNK_START_THREE);
 assert(snap.player_boost.boosts_earned==0);
 assert(snap.player_boost.boost_left_seconds==0);
 assert(snap.player_boost.charge_seconds==0);
 cnk_app_runtime_close(&app);
 /* Regression: the provisional HUMAN ability must not mutate NPC kart
  * physics, AI inputs or trajectories. Two independent sessions from the
  * same original ROM take identical ticks; only one player holds R. */
 cnk_app_runtime with_boost={0},without_boost={0};
 cnk_app_snapshot boosted={0},control={0};
 assert(!cnk_app_runtime_init(&with_boost,&vfs,3,3));
 assert(!cnk_app_runtime_init(&without_boost,&vfs,3,3));
 uint64_t t1=5000000,t2=5000000;
 assert(!cnk_app_runtime_tick(&with_boost,t1,0,driving,&boosted));
 assert(!cnk_app_runtime_tick(&without_boost,t2,0,driving,&control));
 tick(&with_boost,&t1,CNK_UI_CONFIRM,driving,&boosted);
 tick(&without_boost,&t2,CNK_UI_CONFIRM,driving,&control);
 int activated=0;
 for(int i=0;i<360;i++){
  const uint32_t drift=(i>=200&&i<280)?CNK_UI_DRIFT:0;
  tick(&with_boost,&t1,drift,driving,&boosted);
  tick(&without_boost,&t2,0,driving,&control);
  if(boosted.app_events&CNK_APP_EVENT_BOOST_START)activated++;
  assert(boosted.state==control.state);
  assert(with_boost.frontend.game.simulation_frames==
         without_boost.frontend.game.simulation_frames);
  for(uint32_t k=1;k<3;k++){
   const cnk_kart *x=&with_boost.frontend.game.session.racers[k].kart;
   const cnk_kart *y=&without_boost.frontend.game.session.racers[k].kart;
   assert(x->position.x==y->position.x);
   assert(x->position.y==y->position.y);
   assert(x->position.z==y->position.z);
   assert(x->speed==y->speed&&x->heading==y->heading);
   assert(with_boost.frontend.game.session.racers[k].boost.boosts_earned==0);
  }
 }
 assert(activated==1);
 cnk_app_runtime_close(&with_boost);
 cnk_app_runtime_close(&without_boost);
 fufs_close(&vfs);
 puts("PASS: ROM -> countdown -> player boost modifies actual speed -> pause/expiry/restart -> opponents unchanged");
 return 0;
}
