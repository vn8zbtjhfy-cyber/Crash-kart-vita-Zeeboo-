#include "cnk_app_runtime.h"
#include "cnk_launch.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void unit_test(void){
 cnk_launch_state s={0};
 cnk_launch_reset(&s);
 /* High throttle held already at TWO never qualifies in ONE. */
 assert(!cnk_launch_step(&s,CNK_START_TWO,100000,100000,1,0));
 assert(!cnk_launch_step(&s,CNK_START_ONE,500000,500000,1,CNK_START_EVENT_GO));
 assert(s.consumed);
 assert(!cnk_launch_step(&s,CNK_START_ONE,100000,100000,1,CNK_START_EVENT_GO));
 cnk_launch_reset(&s);
 assert(!cnk_launch_step(&s,CNK_START_ONE,600000,200000,1,0));
 assert(s.charged_us==200000);
 assert(!cnk_launch_step(&s,CNK_START_ONE,400000,100000,1,0));
 assert(cnk_launch_step(&s,CNK_START_ONE,300000,300000,1,CNK_START_EVENT_GO)==1);
 assert(!cnk_launch_step(&s,CNK_START_RUNNING,0,200000,1,0));
 cnk_launch_reset(&s);
 assert(!cnk_launch_step(&s,CNK_START_ONE,200000,100000,1,0));
 assert(!cnk_launch_step(&s,CNK_START_ONE,100000,100000,1,CNK_START_EVENT_GO));
 cnk_launch_reset(&s);
 assert(!cnk_launch_step(&s,CNK_START_ONE,500000,200000,1,0));
 assert(!cnk_launch_step(&s,CNK_START_ONE,300000,100000,0,0));
 assert(!cnk_launch_step(&s,CNK_START_ONE,200000,200000,1,CNK_START_EVENT_GO));
 cnk_launch_state saved=s;
 assert(cnk_launch_step(&s,CNK_START_ONE,0,100,NAN,0)<0);
 assert(memcmp(&s,&saved,sizeof(s))==0);
}
static float race(const fufs_file *vfs,int throttle_style){
 cnk_app_runtime a={0};
 assert(!cnk_app_runtime_init(&a,vfs,3,2));
 assert(!cnk_frontend_start(&a.frontend));
 /* This is the exact 3,2,1 schedule. Ten 100 ms ticks per light. */
 for(int i=0;i<30;i++){
  cnk_driver_input in={0};
  in.throttle=(throttle_style==0 ||
       (throttle_style==1 && i>=23) ||
       (throttle_style==2 && i>=28))?1.0f:0.0f;
  assert(cnk_frontend_update(&a.frontend,in,100000)>=0);
  assert(a.frontend.game.simulation_frames==0);
  if(i<29)assert(a.frontend.game.session.racers[0].boost.boost_left_seconds==0);
 }
 assert(a.frontend.start_sequence.phase==CNK_START_GO);
 assert(a.frontend.game.session.racers[0].boost.boost_left_seconds==
   (throttle_style==1?0.75f:0.0f));
 /* Read app events without forcing another sim tick; GO event is pending
  * in cnk_game and appears once when drained. */
 uint32_t events=0;
 assert(!cnk_game_take_boost_events(&a.frontend.game,&events));
 assert(((events&CNK_BOOST_LAUNCH)!=0)==(throttle_style==1));
 assert(((events&CNK_BOOST_ACTIVATED)!=0)==(throttle_style==1));
 assert(!cnk_game_take_boost_events(&a.frontend.game,&events)&&events==0);
 cnk_driver_input accel={1,0,0};
 for(int i=0;i<35;i++)assert(cnk_frontend_update(&a.frontend,accel,16667)>=0);
 float speed=a.frontend.game.session.racers[0].kart.speed;
 /* Reset and restart clear countdown charge and boost. */
 assert(!cnk_frontend_restart(&a.frontend));
 assert(a.frontend.launch.charged_us==0 && !a.frontend.launch.consumed);
 assert(a.frontend.game.session.racers[0].boost.boost_left_seconds==0);
 cnk_app_runtime_close(&a);
 return speed;
}
static void app_events(const fufs_file *vfs){
 cnk_app_runtime app={0}; cnk_app_snapshot snap={0};
 uint64_t clock=5000000u;
 assert(!cnk_app_runtime_init(&app,vfs,1,1));
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 clock+=100000u;
 assert(!cnk_app_runtime_tick(&app,clock,CNK_UI_CONFIRM,(cnk_driver_input){0},&snap));
 assert(snap.app_events&CNK_APP_EVENT_COUNT_3);
 int activations=0;
 for(int i=0;i<45;i++){
  cnk_driver_input input={0};
  input.throttle=(i>=23)?1.0f:0.0f;
  clock+=100000u;
  assert(!cnk_app_runtime_tick(&app,clock,0,input,&snap));
  if(snap.app_events&CNK_APP_EVENT_LAUNCH_BOOST){
   assert(snap.app_events&CNK_APP_EVENT_BOOST_START);
   assert(snap.app_events&CNK_APP_EVENT_GO);
   activations++;
  }
 }
 assert(activations==1);
 cnk_app_runtime_close(&app);
}
int main(int argc,char **argv){
 assert(argc==2);
 unit_test();
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 app_events(&vfs);
 float early=race(&vfs,0),precise=race(&vfs,1),late=race(&vfs,2);
 assert(isfinite(early)&&isfinite(precise)&&isfinite(late));
 assert(precise>early+0.5f);
 assert(fabsf(late-early)<0.001f);
 fufs_close(&vfs);
 puts("PASS: timed GO launch affects real speed, no early/late award, one-shot, no early movement, restart and malformed input");
 return 0;
}
