#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

static void tick(cnk_app_runtime *app,uint64_t us,uint32_t buttons,
                 cnk_app_snapshot *out){
 assert(!cnk_app_runtime_tick(app,us,buttons,(cnk_driver_input){1,0,0},out));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 tick(&app,1000000,0,&snap);
 tick(&app,1010000,CNK_UI_CONFIRM,&snap);
 assert(snap.app_events==(CNK_APP_EVENT_RACE_STARTED|CNK_APP_EVENT_COUNT_3));
 assert(snap.start_phase==CNK_START_THREE && snap.hud.elapsed_ms==0);
 assert(snap.classification.count==3);
 cnk_vec3 start=app.frontend.game.session.racers[0].kart.position;
 uint64_t time=1010000;
 for(int n=1;n<=12;n++){
  time+=250000u;
  tick(&app,time,0,&snap);
  assert(snap.simulated_steps==0 && snap.hud.elapsed_ms==0);
  assert(app.frontend.game.simulation_frames==0);
  const cnk_vec3 *p=&app.frontend.game.session.racers[0].kart.position;
  assert(p->x==start.x && p->y==start.y && p->z==start.z);
  uint32_t expected=n==4?CNK_APP_EVENT_COUNT_2:
                    n==8?CNK_APP_EVENT_COUNT_1:
                    n==12?CNK_APP_EVENT_GO:0;
  assert(snap.app_events==expected);
  if(n==4)assert(snap.start_phase==CNK_START_TWO);
  if(n==8)assert(snap.start_phase==CNK_START_ONE);
  if(n==12)assert(snap.start_phase==CNK_START_GO);
 }
 time+=16667;
 tick(&app,time,0,&snap);
 assert(snap.simulated_steps==1 && snap.hud.elapsed_ms>0);
 assert(snap.app_events==0 && snap.start_phase==CNK_START_GO);
 uint64_t frozen=app.frontend.game.simulation_frames;
 time+=16667;
 tick(&app,time,CNK_UI_PAUSE,&snap);
 assert(snap.state==CNK_FRONTEND_PAUSE && snap.app_events==CNK_APP_EVENT_PAUSED);
 for(int i=0;i<6;i++){
  time+=250000;
  tick(&app,time,0,&snap);
  assert(snap.state==CNK_FRONTEND_PAUSE);
  assert(snap.hud.elapsed_ms==snap.classification.rows[0].elapsed_ms);
  assert(app.frontend.game.simulation_frames==frozen);
  assert(snap.app_events==0);
 }
 time+=10000;tick(&app,time,CNK_UI_CONFIRM,&snap);
 assert(snap.state==CNK_FRONTEND_RACE && snap.app_events==CNK_APP_EVENT_RESUMED);
 assert(app.frontend.game.simulation_frames==frozen);
 /* Restart from pause recreates a fresh countdown and a clean race. */
 time+=10000;tick(&app,time,0,&snap);
 time+=10000;tick(&app,time,CNK_UI_PAUSE,&snap);
 assert(snap.state==CNK_FRONTEND_PAUSE);
 time+=10000;tick(&app,time,0,&snap);
 time+=10000;tick(&app,time,CNK_UI_RESTART,&snap);
 assert(snap.state==CNK_FRONTEND_RACE && snap.start_phase==CNK_START_THREE);
 assert(snap.app_events==(CNK_APP_EVENT_RACE_STARTED|CNK_APP_EVENT_RACE_RESTARTED|CNK_APP_EVENT_COUNT_3));
 assert(app.frontend.game.simulation_frames==0 && snap.hud.elapsed_ms==0);
 assert(snap.classification.count==3);
 /* Finish state only after a completed countdown, and results are readable. */
 for(int n=0;n<12;n++){
  time+=250000;tick(&app,time,0,&snap);
 }
 assert(snap.start_phase==CNK_START_GO);
 app.frontend.game.session.racers[0].race.finished=1;
 app.frontend.game.session.racers[0].finish_ms=app.frontend.game.session.now_ms;
 /* Complete a legitimate, nonzero race frame before refreshing. */
 time+=16667;
 tick(&app,time,0,&snap);
 assert(snap.state==CNK_FRONTEND_RESULTS && snap.app_events==CNK_APP_EVENT_RESULTS);
 assert(snap.classification.count==3);
 int found=0;
 for(unsigned i=0;i<snap.classification.count;i++)
  if(snap.classification.rows[i].racer_slot==0){
   assert(snap.classification.rows[i].finished);found=1;
  }
 assert(found);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS: entire start-to-finish Vita application runtime, countdown events, no early movement, pause, resume, restart, classification");
 return 0;
}
