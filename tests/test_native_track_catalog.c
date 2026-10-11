/* Source-derived selection for ALL real Zeebo scenes, not hardcoded screens. */
#include "cnk_track_catalog.h"
#include "cnk_game.h"
#include "cnk_scene.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int close_xyz(float x,float y){return fabsf(x-y)<0.0001f;}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};
 assert(fufs_open(argv[1],&vfs)==0);
 cnk_track_catalog c={0};
 assert(cnk_track_catalog_build(&vfs,&c)==0);
 assert(c.source==&vfs);
 assert(c.count==14 && c.race_count==12);
 assert(cnk_track_race_at(&c,12)==NULL);
 assert(cnk_track_race_at(&c,UINT32_MAX)==NULL);
 assert(cnk_track_find(&c,0)==NULL);
 const uint32_t expected[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 for(uint32_t i=0;i<c.race_count;i++){
  const cnk_track_entry *e=cnk_track_race_at(&c,i);
  assert(e && e->kind==CNK_TRACK_RACE && e->has_original_start);
  assert(e->vfs_index==expected[i]);
  assert(e->resource_id==vfs.entries[expected[i]].id);
  assert(e->gates>=2 && e->drive_points>=3 && e->route_length>1.0f);
  assert(cnk_track_find(&c,e->vfs_index)==e);
  cnk_game game={0};
  assert(cnk_game_load_catalog_race(&game,&c,i,3,2)==0);
  assert(game.source_spawn_enabled && game.scene_index==e->vfs_index);
  const cnk_kart *kart=&game.session.racers[0].kart;
  assert(close_xyz(kart->position.x,e->original_start.x));
  assert(close_xyz(kart->position.y,e->original_start.y));
  assert(close_xyz(kart->position.z,e->original_start.z));
  cnk_driver_input input={0.5f,0,0};
  assert(cnk_game_update(&game,input,16667)==1);
  assert(isfinite(game.session.racers[0].kart.position.x));
  cnk_game_unload(&game);
 }
 for(uint32_t i=0;i<c.count;i++){
  const cnk_track_entry *e=&c.entries[i];
  assert(cnk_track_find(&c,e->vfs_index)==e);
  if(e->kind==CNK_TRACK_OTHER){
   assert(e->vfs_index==181 || e->vfs_index==289);
  }
 }
 cnk_game absent={0};
 assert(cnk_game_load_catalog_race(&absent,&c,c.race_count,3,2)<0);
 assert(!absent.loaded);
 cnk_track_catalog altered=c;
 altered.entries[0].resource_id^=1u;
 assert(cnk_game_load_catalog_race(&absent,&altered,0,3,2)<0);
 assert(!absent.loaded);
 assert(cnk_track_catalog_build(NULL,&altered)<0);
 assert(altered.count==0);
 assert(cnk_track_catalog_build(&vfs,NULL)<0);
 fufs_close(&vfs);
 printf("PASS source track catalog: %u scenes, %u verified race routes, 2 non-race scenes, 12 original STRT game loads. No invented track names.\n",c.count,c.race_count);
 return 0;
}
