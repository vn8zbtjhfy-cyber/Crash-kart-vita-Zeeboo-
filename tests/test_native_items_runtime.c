#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void step(cnk_app_runtime *app,uint64_t *clock,uint32_t buttons,
                 cnk_driver_input drive,cnk_app_snapshot *out){
 *clock+=16667u;
 assert(!cnk_app_runtime_tick(app,*clock,buttons,drive,out));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime a={0};cnk_app_snapshot sn={0};
 assert(!cnk_app_runtime_init(&a,&vfs,1,3));
 uint64_t clock=1000000;
 cnk_driver_input idle={0,0,0};
 assert(!cnk_app_runtime_tick(&a,clock,0,idle,&sn));
 step(&a,&clock,CNK_UI_CONFIRM,idle,&sn);
 assert(sn.state==CNK_FRONTEND_RACE);
 cnk_game *g=&a.frontend.game;
 cnk_vec3 start=g->session.racers[0].kart.position;
 assert(!cnk_game_place_item(g,CNK_PICKUP_WUMPA,start,1.0f,CNK_WEAPON_NONE));
 assert(!cnk_game_place_item(g,CNK_PICKUP_CRATE,start,1.0f,CNK_WEAPON_TURBO));
 /* L before GO cannot fire a weapon or bypass the countdown. */
 for(int i=0;i<120;i++){
  step(&a,&clock,CNK_UI_ITEM,idle,&sn);
  assert(sn.hud.elapsed_ms==0 && sn.player_items.wumpa==0 &&
         sn.player_items.held==CNK_WEAPON_NONE);
 }
 int crates=0,fruits=0;
 for(int i=0;i<160;i++){
  step(&a,&clock,0,idle,&sn);
  if(sn.app_events&CNK_APP_EVENT_ITEM_COLLECTED)crates++;
  if(sn.app_events&CNK_APP_EVENT_WUMPA_COLLECTED)fruits++;
 }
 assert(crates==1 && fruits==1);
 assert(sn.player_items.held==CNK_WEAPON_TURBO && sn.player_items.wumpa==1);
 assert(sn.placed_pickups==2);
 assert(g->session.items.racers[0].turbo_seconds==0);
 step(&a,&clock,CNK_UI_ITEM,idle,&sn);
 assert(sn.app_events&CNK_APP_EVENT_ITEM_FIRED);
 assert(sn.player_items.held==CNK_WEAPON_NONE);
 assert(sn.player_items.turbo_seconds>0);
 float time=sn.player_items.turbo_seconds;
 step(&a,&clock,CNK_UI_PAUSE,idle,&sn);
 assert(sn.state==CNK_FRONTEND_PAUSE);
 for(int i=0;i<100;i++){
  step(&a,&clock,CNK_UI_ITEM,idle,&sn);
  assert(sn.player_items.turbo_seconds==time);
 }
 step(&a,&clock,CNK_UI_PAUSE,idle,&sn);
 assert(sn.state==CNK_FRONTEND_RACE);
 step(&a,&clock,0,idle,&sn);
 assert(sn.player_items.turbo_seconds<time);
 /* Restart is a true reset; the manually placed test fixtures are removed. */
 step(&a,&clock,CNK_UI_PAUSE,idle,&sn);
 step(&a,&clock,0,idle,&sn);
 step(&a,&clock,CNK_UI_RESTART,idle,&sn);
 assert(sn.state==CNK_FRONTEND_RACE && sn.start_phase==CNK_START_THREE);
 assert(sn.player_items.held==CNK_WEAPON_NONE && sn.player_items.wumpa==0);
 assert(sn.player_items.turbo_seconds==0 && sn.placed_pickups==0);
 cnk_app_runtime_close(&a);
 fufs_close(&vfs);
 puts("PASS: authentic ROM track -> 3-2-1 -> source-pose test pickups -> L item -> pause -> restart clean");
}
