#include "cnk_app_runtime.h"
#include "cnk_minimap.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int near(float a,float b){return fabsf(a-b)<0.02f;}
static int step(cnk_app_runtime *app,uint64_t us,uint32_t b,cnk_app_snapshot *o){
 return cnk_app_runtime_tick(app,us,b,(cnk_driver_input){1,0,0},o);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!step(&app,1000000,0,&snap));
 assert(!step(&app,1010000,CNK_UI_CONFIRM,&snap));
 assert(snap.state==CNK_FRONTEND_RACE);
 assert(snap.minimap.path_count==CNK_MINIMAP_PATH_POINTS);
 assert(snap.minimap.racer_count==3);
 for(unsigned i=0;i<snap.minimap.path_count;i++){
  assert(isfinite(snap.minimap.path[i].x)&&isfinite(snap.minimap.path[i].z));
  assert(snap.minimap.path[i].x>=0&&snap.minimap.path[i].x<=1);
  assert(snap.minimap.path[i].z>=0&&snap.minimap.path[i].z<=1);
 }
 cnk_session *s=&app.frontend.game.session;
 cnk_vec3 start=s->racers[0].recovery_kart.position;
 s->racers[0].kart.position.x=start.x+42;
 s->racers[0].kart.position.z=start.z+35;
 assert(!step(&app,1020000,CNK_UI_RECOVER,&snap));
 assert(snap.hud.recoveries==0); /* Never during countdown */
 assert(!near(s->racers[0].kart.position.x,start.x));
 uint64_t us=1020000;
 for(int i=0;i<14;i++){
  us+=250000;
  assert(!step(&app,us,0,&snap));
 }
 assert(cnk_start_sequence_racing(&app.frontend.start_sequence));
 /* Reset must not advance the lap/clock, erase inventory or move rivals. */
 s->racers[0].kart.position.x+=90;
 s->racers[0].kart.position.z-=85;
 cnk_vec3 ai=s->racers[1].kart.position;
 uint32_t gate=s->racers[0].race.next_gate;
 uint32_t lap=s->racers[0].race.completed_laps;
 uint64_t ms=s->now_ms;
 s->items.racers[0].wumpa=5;
 assert(!step(&app,us+1000,CNK_UI_RECOVER,&snap));
 assert(snap.app_events&CNK_APP_EVENT_KART_RECOVERED);
 assert(snap.hud.recoveries==1);
 assert(near(s->racers[0].kart.position.x,start.x));
 assert(near(s->racers[0].kart.position.z,start.z));
 assert(s->racers[0].race.completed_laps==lap);
 assert(s->racers[0].race.next_gate==gate);
 assert(s->now_ms==ms); /* sub-frame event did not tick the clock */
 assert(s->items.racers[0].wumpa==5);
 assert(near(s->racers[1].kart.position.x,ai.x));
 assert(near(s->racers[1].kart.position.z,ai.z));
 assert(snap.minimap.path_count==CNK_MINIMAP_PATH_POINTS);
 /* Holding Circle must not reset again; releasing then pressing may. */
 assert(!step(&app,us+2000,CNK_UI_RECOVER,&snap));
 assert(snap.hud.recoveries==1);
 assert(!(snap.app_events&CNK_APP_EVENT_KART_RECOVERED));
 assert(!step(&app,us+3000,CNK_UI_PAUSE,&snap));
 assert(snap.state==CNK_FRONTEND_PAUSE);
 assert(!step(&app,us+4000,CNK_UI_RECOVER,&snap));
 assert(snap.hud.recoveries==1);
 assert(!step(&app,us+5000,0,&snap));
 assert(!step(&app,us+6000,CNK_UI_CONFIRM,&snap));
 assert(snap.state==CNK_FRONTEND_RACE);
 assert(!step(&app,us+7000,0,&snap));
 assert(!step(&app,us+8000,CNK_UI_RECOVER,&snap));
 assert(snap.hud.recoveries==2);
 cnk_minimap_frame before=snap.minimap;
 cnk_vec3 saved=s->racers[1].kart.position;
 s->racers[1].kart.position.x=INFINITY;
 assert(cnk_minimap_build(s,&snap.minimap)<0);
 s->racers[1].kart.position=saved;
 assert(!cnk_minimap_build(s,&snap.minimap));
 assert(snap.minimap.path_count==before.path_count);
 /* Source route never depends on source kart pose. */
 for(int i=0;i<96;i++){
  assert(near(snap.minimap.path[i].x,before.path[i].x));
  assert(near(snap.minimap.path[i].z,before.path[i].z));
 }
 cnk_app_runtime_close(&app);
 /* Every original source-backed race layout must produce a finite
  * minimap, not only the first track used by the frontend test. */
 cnk_track_catalog catalog={0};
 assert(!cnk_track_catalog_build(&vfs,&catalog));
 assert(catalog.race_count>=1);
 for(uint32_t j=0;j<catalog.race_count;j++){
  cnk_game other={0};
  assert(!cnk_game_load_catalog_race(&other,&catalog,j,3,2));
  cnk_minimap_frame route=other.route_minimap;
  assert(route.path_count==96&&route.racer_count==3);
  for(unsigned k=0;k<96;k++){
   assert(isfinite(route.path[k].x)&&isfinite(route.path[k].z));
   assert(route.path[k].x>=0&&route.path[k].x<=1);
   assert(route.path[k].z>=0&&route.path[k].z<=1);
  }
  cnk_game_unload(&other);
 }
 fufs_close(&vfs);
 puts("PASS recovery: GO gating, source STRT reset, no synthetic gate/lap, monotonic timer, inventory, rivals, pause, event, edge; source DRV0 minimap");
 return 0;
}
