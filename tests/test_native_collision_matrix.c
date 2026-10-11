/* Exercise the REAL VFS-backed track / collision geometry across all twelve
 * validated races, with and without experimental contact algorithms.
 * Passing is stability only: it does not prove original Zeebo material flags. */
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void run_case(const fufs_file *vfs,const cnk_track_catalog *catalog,
                     uint32_t track,unsigned flags){
 cnk_game g={0};
 assert(cnk_game_load_catalog_race(&g,catalog,track,3,2)==0);
 assert(g.loaded);
 assert(!cnk_game_enable_provisional_walls(&g,(int)(flags&1u)));
 assert(!cnk_game_enable_provisional_kart_contacts(&g,(int)((flags>>1)&1u)));
 assert((unsigned)g.session.provisional_walls_enabled==(flags&1u));
 assert((unsigned)g.session.provisional_kart_contacts_enabled==((flags>>1)&1u));
 assert(g.collision.mesh==&g.geometry && g.collision.count>0);
 uint64_t frame0=g.simulation_frames;
 for(unsigned frame=0;frame<180;frame++){
  /* Player follows the original route controller; no invented shortcuts. */
  cnk_driver_input player={0};
  if(!g.session.racers[0].race.finished)
   assert(!cnk_ai_controls(&g.session.racers[0].ai,&g.session.racers[0].kart,
                           &g.session.tuning,&player));
  int steps=cnk_game_update(&g,player,16667);
  assert(steps==1);
  assert(g.simulation_frames==frame0+frame+1u);
  assert(g.session.racer_count==3);
  unsigned mask=0;
  for(unsigned slot=0;slot<3;slot++){
   const cnk_session_racer *r=&g.session.racers[slot];
   assert(isfinite(r->kart.position.x));
   assert(isfinite(r->kart.position.y));
   assert(isfinite(r->kart.position.z));
   assert(isfinite(r->kart.speed));
   assert(r->race.completed_laps<=2u);
   int rank=cnk_session_position(&g.session,slot);
   assert(rank>=1 && rank<=3);
   mask|=1u<<(unsigned)rank;
  }
  assert(mask==0xEu);
 }
 cnk_game_hud hud={0};
 assert(!cnk_game_hud_read(&g,&hud));
 assert(hud.elapsed_ms==3000u && hud.competitors==3u);
 /* Pausing stops both experimental contacts and the source race clock. */
 assert(!cnk_game_pause(&g,1));
 assert(!cnk_game_update(&g,(cnk_driver_input){0},250000));
 assert(g.simulation_frames==180);
 assert(!cnk_game_pause(&g,0));
 assert(cnk_game_update(&g,(cnk_driver_input){0},16667)==1);
 printf("source scene %u flags=%u: 181 fixed simulation frames PASS\n",
        cnk_track_race_at(catalog,track)->vfs_index,flags);
 cnk_game_unload(&g);
 (void)vfs;
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};
 assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog cat={0};
 assert(!cnk_track_catalog_build(&vfs,&cat));
 assert(cat.race_count==12);
 for(uint32_t i=0;i<cat.race_count;i++)
  for(unsigned mode=0;mode<4;mode++)run_case(&vfs,&cat,i,mode);
 fufs_close(&vfs);
 puts("PASS 48 source circuit/contact configurations, 8688 physical frames");
 return 0;
}
