#include "cnk_game.h"
#include "cnk_items.h"
#include "cnk_collision.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846f

static void reverse_controls(void){
 cnk_drive_tuning t={0};cnk_drive_default_tuning(&t);
 cnk_kart k={0};assert(!cnk_kart_spawn(&k,(cnk_vec3){0,0,0},0));
 cnk_driver_input brake={0,1,0};
 for(unsigned i=0;i<120;i++)assert(!cnk_drive_step(&k,&t,brake,1.0f/60.0f,NULL));
 assert(k.speed < -6.0f && k.speed >= -t.reverse_speed-0.0001f);
 assert(k.position.z< -1.0f); /* the vehicle really moved backward */
 for(unsigned i=0;i<10;i++)assert(!cnk_drive_step(&k,&t,(cnk_driver_input){0,1,0.8f},1.0f/60.0f,NULL));
 assert(k.heading<0.0f); /* steering sense reverses as the kart backs up */
 float prior=k.speed;
 assert(!cnk_drive_step(&k,&t,(cnk_driver_input){0,0,0},1.0f/60.0f,NULL));
 assert(k.speed>prior); /* no-brake drag slowly returns to a full stop */
 for(unsigned i=0;i<100;i++)assert(!cnk_drive_step(&k,&t,(cnk_driver_input){1,0,0},1.0f/60.0f,NULL));
 assert(k.speed>0.0f); /* gas always drives forwards, even after reversing */
 assert(!cnk_kart_spawn(&k,(cnk_vec3){0,0,0},0));
 for(unsigned i=0;i<90;i++)assert(!cnk_drive_step(&k,&t,(cnk_driver_input){1,1,0},1.0f/60.0f,NULL));
 assert(k.speed>=0.0f); /* simultaneous pedal hold never generates reverse */
}

static void wall_projectiles(void){
 /* One source-like upright wall plus a separate flat ground plane. The
  * synthetic triangles are ONLY used for controlled collision testing. */
 cnk_geo_triangle triangles[3]={0};
 triangles[0].v[0]=(cnk_geo_vertex){.x=-5,.y=-1,.z=-5};
 triangles[0].v[1]=(cnk_geo_vertex){.x=5,.y=-1,.z=-5};
 triangles[0].v[2]=(cnk_geo_vertex){.x=0,.y=-1,.z=5};
 triangles[1].v[0]=(cnk_geo_vertex){.x=0,.y=-2,.z=-3};
 triangles[1].v[1]=(cnk_geo_vertex){.x=0,.y=3,.z=-3};
 triangles[1].v[2]=(cnk_geo_vertex){.x=0,.y=-2,.z=3};
 triangles[2].v[0]=(cnk_geo_vertex){.x=0,.y=3,.z=-3};
 triangles[2].v[1]=(cnk_geo_vertex){.x=0,.y=3,.z=3};
 triangles[2].v[2]=(cnk_geo_vertex){.x=0,.y=-2,.z=3};
 cnk_geometry geom={0};geom.triangles=triangles;geom.triangle_count=3;
 cnk_collision_grid grid={0};assert(!cnk_collision_grid_build(&geom,4,4,&grid));
 cnk_segment_hit check={0};
 assert(!cnk_collision_segment_cast_steep(&grid,(cnk_vec3){-1,.4f,0},
                                         (cnk_vec3){1,.4f,0},.55f,&check));
 assert(check.fraction>0.40f && check.fraction<0.60f);
 for(unsigned kind=0;kind<2;kind++){
  cnk_kart karts[CNK_ITEM_MAX_RACERS]={0};
  cnk_vec3 prev[CNK_ITEM_MAX_RACERS]={0};
  uint8_t moving[CNK_ITEM_MAX_RACERS]={1,1};
  uint32_t ev[CNK_ITEM_MAX_RACERS]={0};
  cnk_item_world blocked={0},legacy={0};
  assert(!cnk_kart_spawn(&karts[0],(cnk_vec3){-1,0,0},PI/2));
  assert(!cnk_kart_spawn(&karts[1],
                        (cnk_vec3){kind?0.9f:0.7f,0,0},0));
  for(unsigned i=0;i<2;i++)prev[i]=karts[i].position;
  if(kind){
   blocked.missiles[0]=(cnk_item_missile){{-1,0,0},PI/2,2,0,UINT32_MAX,1};
  }else{
   blocked.bombs[0]=(cnk_item_bomb){{-1,0,0},PI/2,2,0,1};
  }
  legacy=blocked;
  /* Existing mode: same projectile can pass the wall and hit the kart. */
  assert(!cnk_items_step(&legacy,karts,prev,moving,2,.10f,ev));
  assert(ev[1]&CNK_ITEM_EVENT_HIT);
  assert(karts[1].speed==0); /* the impact modifies velocity, but zero stays zero */
  memset(ev,0,sizeof(ev));
  assert(!cnk_items_step_world(&blocked,karts,prev,moving,2,.10f,ev,&grid,.55f));
  assert(!(ev[1]&CNK_ITEM_EVENT_HIT));
  if(kind){
   assert(!blocked.missiles[0].active);
   assert(ev[0]&CNK_ITEM_EVENT_MISSILE_WALL);
  }else{
   assert(!blocked.bombs[0].active);
   assert(ev[0]&CNK_ITEM_EVENT_BOMB_WALL);
  }
  assert(!(ev[1]&CNK_ITEM_EVENT_HIT));
 }
 /* Incorrect validation cannot damage a projectile or player state. */
 cnk_item_world original={0},copy={0};
 original.bombs[0]=(cnk_item_bomb){{-1,0,0},PI/2,2,0,1};copy=original;
 cnk_kart karts[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t moving[CNK_ITEM_MAX_RACERS]={1};uint32_t ev[CNK_ITEM_MAX_RACERS]={0};
 assert(cnk_items_step_world(&original,karts,before,moving,1,.016f,ev,&grid,1.1f)<0);
 assert(!memcmp(&original,&copy,sizeof(original)));
 /* Malformed grid on a live bomb must leave all actors unchanged. */
 cnk_collision_grid bad_grid=grid;
 cnk_geometry bad_geom=geom;bad_geom.triangle_count=0;
 bad_grid.mesh=&bad_geom;
 cnk_item_world live={0},saved={0};
 live.bombs[0]=(cnk_item_bomb){{-1,0,0},PI/2,2,0,1};saved=live;
 cnk_kart old_karts[CNK_ITEM_MAX_RACERS]={0};
 memcpy(old_karts,karts,sizeof(old_karts));
 assert(cnk_items_step_world(&live,karts,before,moving,1,.10f,ev,&bad_grid,.55f)<0);
 assert(!memcmp(&live,&saved,sizeof(live)));
 assert(!memcmp(&old_karts,karts,sizeof(karts)));
 cnk_collision_grid_free(&grid);
}

static void original_vfs_integration(const char *rom){
 fufs_file file={0};assert(!fufs_open(rom,&file));
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(&file,&catalog));
 assert(catalog.race_count==12);
 /* The original source scene is loaded; this is not a synthetic circuit. */
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&catalog,0,1,1));
 cnk_game_hud hud={0};assert(!cnk_game_hud_read(&g,&hud));
 assert(!hud.reversing && hud.kart_speed==0.0f);
 for(unsigned i=0;i<120;i++){
  int frames=cnk_game_update(&g,(cnk_driver_input){0,1,0},16667);
  assert(frames>=0);
 }
 assert(!cnk_game_hud_read(&g,&hud));
 assert(hud.reversing && hud.kart_speed<0.0f);
 assert(!hud.finished && hud.lap==0);
 float previous=g.session.racers[0].kart.speed;
 assert(!cnk_game_pause(&g,1));
 assert(cnk_game_update(&g,(cnk_driver_input){0,1,0},200000)==0);
 assert(g.session.racers[0].kart.speed==previous);
 assert(!cnk_game_pause(&g,0));
 /* Enable experimental source-wall mode for real track geometry, exercise
  * the entire native frame path, then return to the unchanged baseline. */
 assert(!cnk_session_enable_provisional_walls(&g.session,1));
 for(unsigned i=0;i<60;i++)assert(cnk_game_update(&g,(cnk_driver_input){0,0,0},16667)>=0);
 assert(!cnk_session_enable_provisional_walls(&g.session,0));
 cnk_game_unload(&g);
 fufs_close(&file);
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 reverse_controls();wall_projectiles();original_vfs_integration(argv[1]);
 puts("PASS: reverse, wall-clipped projectiles, unchanged legacy mode and actual VFS race");
 return 0;
}
