#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static int near(float a,float b){return fabsf(a-b)<0.00015f;}
static float angle_delta(float a,float b){return atan2f(sinf(a-b),cosf(a-b));}

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(fufs_open(argv[1],&vfs)==0);
 const uint32_t race_scenes[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 for(unsigned si=0;si<12;si++){
  cnk_game game={0};uint32_t idx=race_scenes[si];
  assert(cnk_game_load_original_start(&game,&vfs,idx,3,2)==0);
  assert(game.source_spawn_enabled && game.session.active);
  cnk_vec3 marker={0};float heading=0;
  assert(cnk_scene_start_pose(&game.scene,&marker,&heading)==0);
  const cnk_kart *kart=&game.session.racers[0].kart;
  assert(near(kart->position.x,marker.x));
  assert(near(kart->position.y,marker.y));
  assert(near(kart->position.z,marker.z));
  assert(fabsf(angle_delta(kart->heading,heading))<0.00015f);
  cnk_nav_projection proj={0};
  assert(cnk_nav_nearest(game.session.route,marker,1,&proj)==0);
  assert(proj.distance<20.0f);
  assert(near(game.session.racers[0].route_fraction,proj.fraction));
  cnk_game_hud hud={0};
  assert(cnk_game_hud_read(&game,&hud)==0);
  assert(hud.lap==0 && hud.competitors==3);
  /* Smoke test of native update from the ACTUAL source position. Physics
   * coefficients and AI are not the Zeebo originals. */
  cnk_driver_input in={0.4f,0.0f,0.0f};
  for(int frame=0;frame<120;frame++){
   assert(cnk_game_update(&game,in,16667)==1);
   for(unsigned k=0;k<3;k++){
    cnk_kart *r=&game.session.racers[k].kart;
    assert(isfinite(r->position.x)&&isfinite(r->position.y)&&isfinite(r->position.z));
    assert(isfinite(r->speed)&&isfinite(r->heading));
   }
  }
  assert(game.simulation_frames==120);
  assert(cnk_game_hud_read(&game,&hud)==0);
  assert(hud.elapsed_ms==2000);
  assert(cnk_game_pause(&game,1)==0);
  assert(cnk_game_update(&game,in,16667)==0);
  assert(cnk_game_pause(&game,0)==0);
  /* Full original-start round trip. The player uses route-based AI for the
   * test only, not the control model of the original game. A source STRT
   * point must not strand the kart outside the checkpoint sequence. */
  uint32_t frames=0;
  const int smoke=getenv("CNK_SPAWN_SMOKE")!=NULL;
  for(;!smoke && frames<36000;frames++){
   cnk_driver_input ai={0};
   if(!game.session.racers[0].race.finished)
    assert(cnk_ai_controls(&game.session.racers[0].ai,
            &game.session.racers[0].kart,&game.session.tuning,&ai)==0);
   assert(cnk_game_update(&game,ai,16667)==1);
   if(game.session.racers[0].race.finished &&
      game.session.racers[1].race.finished &&
      game.session.racers[2].race.finished)break;
  }
  if(!smoke){
   assert(frames<36000);
   for(unsigned k=0;k<3;k++){
    assert(game.session.racers[k].race.finished);
    assert(game.session.racers[k].race.completed_laps==2);
   }
   uint32_t events=0;
   assert(cnk_game_take_events(&game,0,&events)==0);
   assert((events&(CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH))==
          (CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH));
  }
  printf("[original STRT %u] %.3f,%.3f,%.3f heading %.3f; %s, %u frames\n",
         idx,marker.x,marker.y,marker.z,heading,
         smoke?"smoke test":"2 laps",frames+120u);
  cnk_game_unload(&game);
 }
 /* Special scenes without a valid full racing route must not be mistaken
  * for regular tracks merely because they contain source resources. */
 for(unsigned i=0;i<2;i++){
  const uint32_t idx=i==0?181:289;
  cnk_game game={0};
  assert(cnk_game_load_original_start(&game,&vfs,idx,3,2)<0);
  assert(!game.loaded);
 }
 fufs_close(&vfs);
 if(getenv("CNK_SPAWN_SMOKE")) puts("PASS: sanitized authentic STRT pose smoke tests");
 else puts("PASS: 12 authentic STRT poses, direction, 2 full laps per racer each");
 puts("CAVEAT: competitor grid and gameplay physics still provisional.");
 return 0;
}
