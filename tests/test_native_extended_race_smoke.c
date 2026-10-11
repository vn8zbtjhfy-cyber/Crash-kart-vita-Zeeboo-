/* A longer source-data end-to-end race soak across all original race tracks.
 * This does NOT certify original gameplay or PS Vita hardware. */
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void exercise(const cnk_track_catalog *catalog,unsigned track,unsigned mode){
 cnk_game game={0};
 assert(cnk_game_load_catalog_race(&game,catalog,track,3,2)==0);
 assert(cnk_game_enable_provisional_walls(&game,(int)(mode&1u))==0);
 assert(cnk_game_enable_provisional_kart_contacts(&game,(int)((mode>>1)&1u))==0);
 uint64_t last_frame=game.simulation_frames;
 for(unsigned n=0;n<600;n++){
  cnk_driver_input input={0};
  if(!game.session.racers[0].race.finished)
   assert(cnk_ai_controls(&game.session.racers[0].ai,
        &game.session.racers[0].kart,&game.session.tuning,&input)==0);
  if(n==200||n==400){
   assert(cnk_game_pause(&game,1)==0);
   assert(cnk_game_update(&game,input,16667)==0);
   assert(game.simulation_frames==last_frame);
   assert(cnk_game_pause(&game,0)==0);
  }
  assert(cnk_game_update(&game,input,16667)==1);
  last_frame++;
  assert(game.simulation_frames==last_frame);
  if((n%30)==0 || n==599){
   cnk_game_classification c={0};cnk_game_hud hud={0};
   assert(cnk_game_classification_read(&game,&c)==0);
   assert(cnk_game_hud_read(&game,&hud)==0);
   assert(c.count==3 && hud.competitors==3);
   unsigned seen=0;
   for(unsigned k=0;k<3;k++){
    const cnk_game_classification_row *r=&c.rows[k];
    assert(r->rank==k+1);
    assert(r->racer_slot<3 && !(seen&(1u<<r->racer_slot)));
    seen|=1u<<r->racer_slot;
    const cnk_session_racer *driver=&game.session.racers[r->racer_slot];
    assert(isfinite(driver->kart.position.x));
    assert(isfinite(driver->kart.position.y));
    assert(isfinite(driver->kart.position.z));
    assert(isfinite(driver->kart.speed));
    assert(r->completed_laps<=2);
   }
   assert(seen==7);
  }
 }
 cnk_game_unload(&game);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_track_catalog catalog={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_track_catalog_build(&vfs,&catalog)==0);
 assert(catalog.race_count==12);
 for(unsigned track=0;track<12;track++)for(unsigned mode=0;mode<4;mode++)
  exercise(&catalog,track,mode);
 fufs_close(&vfs);
 puts("PASS: 48 original-track/contact cases, 28,800 fixed steps and safe pause/resume");
 return 0;
}
