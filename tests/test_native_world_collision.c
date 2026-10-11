/* Provisional kart-versus-source-mesh wall collision tests.
 * These verify mechanics and opt-in behavior, not the original Zeebo physics. */
#include "cnk_drive.h"
#include "cnk_game.h"
#include "fufs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static cnk_geo_vertex point(float x,float y,float z){
 cnk_geo_vertex p={0};p.x=x;p.y=y;p.z=z;return p;
}
static void add_triangle(cnk_geo_triangle *t,cnk_geo_vertex a,cnk_geo_vertex b,cnk_geo_vertex c){
 memset(t,0,sizeof(*t));t->v[0]=a;t->v[1]=b;t->v[2]=c;
}
/* y=0 floor plus wall x=3, -5<=z<=5, -1<=y<=4. */
static void create_fixture(cnk_collision_grid *grid,cnk_geometry *geom,cnk_geo_triangle *tris){
 add_triangle(&tris[0],point(-20,0,-20),point(20,0,-20),point(-20,0,20));
 add_triangle(&tris[1],point(20,0,-20),point(20,0,20),point(-20,0,20));
 add_triangle(&tris[2],point(3,-1,-5),point(3,4,-5),point(3,-1,5));
 add_triangle(&tris[3],point(3,4,-5),point(3,4,5),point(3,-1,5));
 memset(geom,0,sizeof(*geom));geom->triangles=tris;geom->triangle_count=4;
 assert(cnk_collision_grid_build(geom,8,8,grid)==0);
}
static void test_world_sweep(void){
 cnk_geo_triangle tris[4];cnk_geometry geom={0};cnk_collision_grid grid={0};
 create_fixture(&grid,&geom,tris);
 cnk_drive_tuning tuning;cnk_drive_default_tuning(&tuning);
 cnk_world_collision_tuning world;cnk_world_collision_default(&world);
 assert(cnk_world_collision_validate(&world)==0);
 assert(world.max_abs_normal_y<0.9f);
 cnk_world_collision_tuning invalid=world;
 invalid.clearance=NAN;assert(cnk_world_collision_validate(&invalid)<0);
 invalid=world;invalid.probe_height=-1;assert(cnk_world_collision_validate(&invalid)<0);
 cnk_kart original={0},legacy={0},blocked={0};
 assert(cnk_kart_spawn(&original, (cnk_vec3){2.75f,0,0}, 1.57079632679f)==0);
 original.speed=30.0f;legacy=original;blocked=original;
 cnk_driver_input coast={0};
 assert(cnk_drive_step(&legacy,&tuning,coast,1.0f/60.0f,&grid)==0);
 assert(legacy.position.x>3.0f); /* old physics crosses the wall */
 assert(cnk_drive_step_world(&blocked,&tuning,coast,1.0f/60.0f,&grid,&world)==1);
 assert(blocked.position.x<2.81f && blocked.position.x>2.70f);
 cnk_kart reported=original;
 int contact=-99;
 assert(cnk_drive_race_step_world_report(&reported,&tuning,coast,1.0f/60.0f,&grid,&world,NULL,0,&contact)==0);
 assert(contact==1 && reported.speed==0.0f);
 assert(reported.position.x<2.81f);
 assert(blocked.speed==0.0f);
 assert(blocked.ticks==1);
 /* A diagonal collision should preserve wall-parallel movement only when
  * explicitly using the experimental slide policy. */
 cnk_kart diagonal={0},stopped={0},sliding={0};
 assert(cnk_kart_spawn(&diagonal,(cnk_vec3){2.75f,0.0f,0.0f},0.78539816339f)==0);
 diagonal.speed=30.0f;stopped=diagonal;sliding=diagonal;
 assert(cnk_drive_step_world(&stopped,&tuning,coast,1.0f/60.0f,&grid,&world)==1);
 cnk_world_collision_tuning slide_config=world;
 slide_config.slide_on_contact=1;
 assert(cnk_world_collision_validate(&slide_config)==0);
 assert(cnk_drive_step_world(&sliding,&tuning,coast,1.0f/60.0f,&grid,&slide_config)==1);
 assert(sliding.position.x<3.0f && sliding.position.x>2.70f);
 assert(fabsf(sliding.position.x-stopped.position.x)<0.01f);
 assert(sliding.position.z>stopped.position.z+0.09f);
 assert(sliding.speed>0.0f&&sliding.speed<diagonal.speed);
 /* Slide policy still cannot tunnel through a wall when advancing directly. */
 cnk_kart straight=original;
 assert(cnk_drive_step_world(&straight,&tuning,coast,1.0f/60.0f,&grid,&slide_config)==1);
 assert(straight.position.x<=2.81f&&fabsf(straight.position.z)<0.01f);
 assert(straight.speed<0.01f);
 invalid=world;invalid.slide_on_contact=2;
 assert(cnk_world_collision_validate(&invalid)<0);
 invalid=world;invalid.clearance=NAN;
 /* A second frame must not tunnel through the same surface. */
 assert(cnk_drive_step_world(&blocked,&tuning,(cnk_driver_input){1,0,0},1.0f/60.0f,&grid,&world)>=0);
 assert(blocked.position.x<=2.82f);
 /* Sweeping parallel to the wall is not a contact. */
 cnk_kart parallel={0};
 assert(cnk_kart_spawn(&parallel,(cnk_vec3){2.75f,0,0},0)==0);
 parallel.speed=20.0f;
 assert(cnk_drive_step_world(&parallel,&tuning,coast,1.0f/60.0f,&grid,&world)==0);
 assert(fabsf(parallel.position.x-2.75f)<0.01f && parallel.position.z>0.1f);
 /* Move from the far side away from the wall. */
 cnk_kart away={0};
 assert(cnk_kart_spawn(&away,(cnk_vec3){3.5f,0,0},1.57079632679f)==0);
 away.speed=15;
 assert(cnk_drive_step_world(&away,&tuning,coast,1.0f/60.0f,&grid,&world)==0);
 assert(away.position.x>3.6f);
 /* Failure must not mutate kart state or its internal tick. */
 cnk_kart snapshot=away;
 assert(cnk_drive_step_world(&away,&tuning,coast,0.0f,&grid,&world)<0);
 assert(memcmp(&away,&snapshot,sizeof(away))==0);
 assert(cnk_drive_step_world(&away,&tuning,coast,1.0f/60.0f,&grid,&invalid)<0);
 assert(memcmp(&away,&snapshot,sizeof(away))==0);
 /* A long allowed frame sweeps through, so the kart cannot tunnel. */
 cnk_kart fast={0};assert(cnk_kart_spawn(&fast,(cnk_vec3){1.0f,0,0},1.57079632679f)==0);
 fast.speed=30;
 assert(cnk_drive_step_world(&fast,&tuning,coast,0.10f,&grid,&world)==1);
 assert(fast.position.x<2.82f && fast.position.x>2.7f);
 cnk_collision_grid_free(&grid);
}
/* A second wall in the projected path must also block sliding. */
static void test_wall_corner_slide(void){
 cnk_geo_triangle tris[6];cnk_geometry geom={0};cnk_collision_grid grid={0};
 add_triangle(&tris[0],point(-20,0,-20),point(20,0,-20),point(-20,0,20));
 add_triangle(&tris[1],point(20,0,-20),point(20,0,20),point(-20,0,20));
 add_triangle(&tris[2],point(3,-1,-5),point(3,4,-5),point(3,-1,5));
 add_triangle(&tris[3],point(3,4,-5),point(3,4,5),point(3,-1,5));
 add_triangle(&tris[4],point(1,-1,0.3f),point(4,-1,0.3f),point(1,4,0.3f));
 add_triangle(&tris[5],point(4,-1,0.3f),point(4,4,0.3f),point(1,4,0.3f));
 geom.triangles=tris;geom.triangle_count=6;
 assert(cnk_collision_grid_build(&geom,8,8,&grid)==0);
 cnk_drive_tuning tuning;cnk_drive_default_tuning(&tuning);
 cnk_world_collision_tuning cfg;cnk_world_collision_default(&cfg);
 cfg.slide_on_contact=1;
 cnk_kart k={0};
 assert(cnk_kart_spawn(&k,(cnk_vec3){2.75f,0,0},0.78539816339f)==0);
 k.speed=30;
 assert(cnk_drive_step_world(&k,&tuning,(cnk_driver_input){0},1.0f/60.0f,&grid,&cfg)==1);
 assert(k.position.x<3.0f && k.position.z<0.3f);
 /* The projected segment must not overrun either surface. */
 assert(k.position.z>0.08f);
 cnk_collision_grid_free(&grid);
}
static void test_real_tracks(const char *filename){
 fufs_file vfs={0};assert(fufs_open(filename,&vfs)==0);
 unsigned usable=0,other=0;
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *raw=NULL;size_t n=0;
  assert(fufs_extract(&vfs,i,&raw,&n)==0);
  int scene=(n>=12&&!memcmp(raw,"PIFF",4)&&!memcmp(raw+8,"SCEN",4));
  free(raw);if(!scene)continue;
  cnk_game g={0};int rc=cnk_game_load_original_start(&g,&vfs,i,3,2);
  if(rc){assert(rc==-8);other++;continue;}
  assert(!g.session.provisional_walls_enabled);
  assert(!g.session.world_tuning.slide_on_contact);
  assert(cnk_game_enable_provisional_wall_slide(&g,1)==0);
  assert(g.session.world_tuning.slide_on_contact);
  assert(cnk_game_enable_provisional_walls(&g,1)==0);
  assert(g.session.provisional_walls_enabled);
  for(int j=0;j<8;j++){
   assert(cnk_game_update(&g,(cnk_driver_input){1,0,0},16667)==1);
   for(uint32_t k=0;k<g.session.racer_count;k++){
    cnk_kart *kart=&g.session.racers[k].kart;
    assert(isfinite(kart->position.x) && isfinite(kart->position.y) && isfinite(kart->position.z));
    assert(isfinite(kart->speed));
   }
  }
  for(uint32_t k=0;k<g.session.racer_count;k++){
   assert(g.session.racers[k].last_wall_contact==0 || g.session.racers[k].last_wall_contact==1);
   assert(g.session.racers[k].wall_contact_count<=8);
  }
  assert(cnk_game_enable_provisional_walls(&g,0)==0);
  assert(cnk_game_enable_provisional_wall_slide(&g,0)==0);
  assert(!g.session.provisional_walls_enabled && !g.session.world_tuning.slide_on_contact);
  assert(cnk_game_update(&g,(cnk_driver_input){1,0,0},16667)==1);
  for(uint32_t k=0;k<g.session.racer_count;k++)assert(g.session.racers[k].last_wall_contact==0);
  cnk_game_unload(&g);usable++;
 }
 fufs_close(&vfs);
 assert(usable==12 && other==2);
 printf("PASS native WORLD COLLISION: synthetic 3D wall + opt-in/off in %u original racing scenes (%u special scenes skipped). NOT the original collision physics.\n",usable,other);
}
int main(int argc,char **argv){assert(argc==2);test_world_sweep();test_wall_corner_slide();test_real_tracks(argv[1]);return 0;}
