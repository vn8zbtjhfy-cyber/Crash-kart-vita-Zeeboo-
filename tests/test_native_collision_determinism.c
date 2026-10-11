/* Fixed 60Hz simulation must be identical at 30Hz vs 60Hz display cadence,
 * including source-world experimental collisions, on all verified circuits. */
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
static int nearf(float a,float b){return fabsf(a-b)<=0.00005f;}
static void verify_equal(const cnk_game *a,const cnk_game *b){
 assert(a->simulation_frames==b->simulation_frames);
 assert(a->session.now_ms==b->session.now_ms);
 assert(a->session.racer_count==b->session.racer_count);
 assert(a->fractional_units==b->fractional_units);
 for(uint32_t i=0;i<a->session.racer_count;i++){
  const cnk_session_racer *x=&a->session.racers[i],*y=&b->session.racers[i];
  assert(nearf(x->kart.position.x,y->kart.position.x));
  assert(nearf(x->kart.position.y,y->kart.position.y));
  assert(nearf(x->kart.position.z,y->kart.position.z));
  assert(nearf(x->kart.heading,y->kart.heading));
  assert(nearf(x->kart.speed,y->kart.speed));
  assert(nearf(x->kart.steering,y->kart.steering));
  assert(x->race.completed_laps==y->race.completed_laps);
  assert(x->race.next_gate==y->race.next_gate);
  assert(x->wall_contact_count==y->wall_contact_count);
  assert(x->kart_contact_count==y->kart_contact_count);
 }
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file src={0};assert(!fufs_open(argv[1],&src));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&src,&cat));
 assert(cat.race_count==12);
 for(uint32_t track=0;track<cat.race_count;track++)for(int flag=0;flag<4;flag++){
  cnk_game a={0},b={0};
  assert(!cnk_game_load_catalog_race(&a,&cat,track,3,2));
  assert(!cnk_game_load_catalog_race(&b,&cat,track,3,2));
  assert(!cnk_game_enable_provisional_walls(&a,flag&1));
  assert(!cnk_game_enable_provisional_walls(&b,flag&1));
  assert(!cnk_game_enable_provisional_kart_contacts(&a,(flag>>1)&1));
  assert(!cnk_game_enable_provisional_kart_contacts(&b,(flag>>1)&1));
  cnk_driver_input controls={.throttle=0.88f,.steer=0.28f};
  for(int step=0;step<180;step++)assert(cnk_game_update(&a,controls,16667)==1);
  for(int step=0;step<90;step++)assert(cnk_game_update(&b,controls,33334)==2);
  verify_equal(&a,&b);
  cnk_game_unload(&a);cnk_game_unload(&b);
 }
 fufs_close(&src);
 puts("PASS deterministic 30/60Hz display for all 12 original tracks and four contact configurations");
 return 0;
}
