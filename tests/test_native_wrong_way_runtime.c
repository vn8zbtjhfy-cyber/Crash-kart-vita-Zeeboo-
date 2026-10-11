#include "cnk_app_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void tick(cnk_app_runtime *app,uint64_t *now,uint32_t buttons,
                 cnk_driver_input controls,cnk_app_snapshot *snap){
 *now+=16667u;
 assert(!cnk_app_runtime_tick(app,*now,buttons,controls,snap));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 uint64_t now=1000000;
 assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&snap));
 tick(&app,&now,CNK_UI_CONFIRM,(cnk_driver_input){0},&snap);
 assert(snap.state==CNK_FRONTEND_RACE);
 for(int i=0;i<190;i++)tick(&app,&now,0,(cnk_driver_input){0},&snap);
 assert(snap.start_phase==CNK_START_RUNNING||snap.start_phase==CNK_START_GO);
 cnk_session *session=&app.frontend.game.session;
 const cnk_sline *route=session->route;
 assert(route&&route->points&&route->count>2);
 /* Set up a deterministic test position ON one of the actual source segments.
  * The original racer grid and engine constants remain untouched on disk. */
 uint32_t chosen=0;float longest=0;
 for(uint32_t j=0;j<route->count;j++){
  cnk_vec3 a=route->points[j],b=route->points[(j+1)%route->count];
  float length=hypotf(b.x-a.x,b.z-a.z);
  if(length>longest){longest=length;chosen=j;}
 }
 assert(longest>3.0f);
 cnk_vec3 a=route->points[chosen],b=route->points[(chosen+1)%route->count];
 cnk_kart *kart=&session->racers[0].kart;
 kart->position=(cnk_vec3){a.x+.65f*(b.x-a.x),a.y+.65f*(b.y-a.y),a.z+.65f*(b.z-a.z)};
 kart->heading=atan2f(a.x-b.x,a.z-b.z);
 kart->speed=8;
 kart->steering=0;
 cnk_route_direction *direction=&session->racers[0].course_direction;
 direction->reverse_seconds=0.79f;
 direction->forward_seconds=0;
 direction->wrong_way=0;
 tick(&app,&now,0,(cnk_driver_input){1,0,0},&snap);
 assert(snap.hud.wrong_way);
 assert(snap.app_events&CNK_APP_EVENT_WRONG_WAY_START);
 uint64_t sim=app.frontend.game.simulation_frames;
 tick(&app,&now,CNK_UI_PAUSE,(cnk_driver_input){1,0,0},&snap);
 assert(snap.state==CNK_FRONTEND_PAUSE);
 assert(snap.hud.wrong_way);
 for(int i=0;i<15;i++){
  tick(&app,&now,0,(cnk_driver_input){1,0,0},&snap);
  assert(snap.hud.wrong_way);
  assert(!(snap.app_events&CNK_APP_EVENT_WRONG_WAY_CLEAR));
  assert(app.frontend.game.simulation_frames==sim);
 }
 /* Pause -> restart resets advisory exactly like other race-local states. */
 tick(&app,&now,CNK_UI_RESTART,(cnk_driver_input){0},&snap);
 assert(snap.state==CNK_FRONTEND_RACE);
 assert(!snap.hud.wrong_way);
 assert(!(snap.app_events&CNK_APP_EVENT_WRONG_WAY_CLEAR));
 assert(session!=NULL);
 cnk_app_runtime_close(&app);
 fufs_close(&vfs);
 puts("PASS: actual ROM DRV0 -> backward kart motion -> one warning cue; pause stable; restart resets without false clear");
}
