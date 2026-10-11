/* Development starting grid safety on all 12 authentic Zeebo race scenes.
 * The real opponent grid is not recovered. This test prevents accidental
 * overlap, fake checkpoint awards or changes to the authenticated STRT pose.
 */
#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static float planar_distance(cnk_vec3 a,cnk_vec3 b){
 float dx=a.x-b.x,dz=a.z-b.z;return sqrtf(dx*dx+dz*dz);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file source={0};assert(fufs_open(argv[1],&source)==0);
 const unsigned scenes[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 for(unsigned s=0;s<12;s++){
  cnk_game game={0};assert(cnk_game_load_original_start(&game,&source,scenes[s],6,2)==0);
  cnk_vec3 start={0};float heading=0;
  assert(cnk_scene_start_pose(&game.scene,&start,&heading)==0);
  assert(planar_distance(game.session.racers[0].kart.position,start)<0.0005f);
  assert(fabsf(atan2f(sinf(game.session.racers[0].kart.heading-heading),
                       cosf(game.session.racers[0].kart.heading-heading)))<0.0005f);
  for(unsigned i=0;i<6;i++){
   const cnk_session_racer *a=&game.session.racers[i];
   assert(a->race.completed_laps==0 && !a->race.finished);
   assert(isfinite(a->kart.heading) && isfinite(a->route_fraction));
   for(unsigned j=i+1;j<6;j++){
    const cnk_session_racer *b=&game.session.racers[j];
    assert(planar_distance(a->kart.position,b->kart.position)>3.0f);
   }
  }
  /* Explicitly enabled test collisions are safe at spawn; never enable
   * unverified collision/material rules by default in the native game. */
  assert(cnk_game_enable_provisional_kart_contacts(&game,1)==0);
  cnk_driver_input idle={0};
  for(unsigned f=0;f<120;f++){
   assert(cnk_game_update(&game,idle,16667)==1);
   for(unsigned i=0;i<6;i++){
    const cnk_session_racer *r=&game.session.racers[i];
    assert(isfinite(r->kart.position.x) && isfinite(r->kart.position.y));
    assert(isfinite(r->kart.position.z) && isfinite(r->kart.speed));
   }
  }
  cnk_game_unload(&game);
 }
 fufs_close(&source);
 puts("PASS: 12 original STRT markers, 6 non-overlapping source-route rival poses and 120 contact-enabled steps each");
 puts("CAVEAT: opponent grid is a development fallback, not Zeebo's original grid");
 return 0;
}
