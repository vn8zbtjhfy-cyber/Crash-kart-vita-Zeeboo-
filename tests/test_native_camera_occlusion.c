/* Camera occlusion regression using real source SCEN geometry and a small
 * deterministic wall for an exact expected camera displacement. */
#include "cnk_camera.h"
#include "cnk_game.h"
#include "cnk_geometry.h"
#include "cnk_collision.h"
#include "fufs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int almost(float a,float b){return fabsf(a-b)<0.001f;}
static void wall_test(void){
 cnk_geo_triangle tris[2]={0};
 tris[0].v[0]=(cnk_geo_vertex){-5,-5,-4,0,0,0};
 tris[0].v[1]=(cnk_geo_vertex){ 5,-5,-4,0,0,0};
 tris[0].v[2]=(cnk_geo_vertex){-5, 8,-4,0,0,0};
 tris[1].v[0]=tris[0].v[1];
 tris[1].v[1]=(cnk_geo_vertex){ 5, 8,-4,0,0,0};
 tris[1].v[2]=tris[0].v[2];
 cnk_geometry geom={0};geom.triangles=tris;geom.triangle_count=2;
 cnk_collision_grid grid={0};
 assert(cnk_collision_grid_build(&geom,8,8,&grid)==0);
 cnk_kart kart={0};kart.heading=0.0f;
 cnk_camera plain={0},occluded={0},empty={0},sentinel={0};
 assert(cnk_camera_follow(&plain,&kart,8,4,4,1.1f,960.0f/544.0f)==0);
 assert(almost(plain.eye.z,-8));
 assert(cnk_camera_follow_obstructed(&occluded,&kart,&grid,8,4,4,1.1f,960.0f/544.0f,0.7f)==0);
 /* On the near side of z=-4, pulled back by world-space margin. */
 assert(occluded.eye.z>-4.0f && occluded.eye.z<-2.5f);
 assert(isfinite(occluded.view_projection[0]));
 assert(cnk_camera_follow_obstructed(&empty,&kart,NULL,8,4,4,1.1f,960.0f/544.0f,0.7f)==0);
 assert(almost(empty.eye.x,plain.eye.x)&&almost(empty.eye.y,plain.eye.y)
        &&almost(empty.eye.z,plain.eye.z));
 /* Invalid inputs must not mutate an existing camera. */
 sentinel=occluded;
 assert(cnk_camera_follow_obstructed(&occluded,&kart,&grid,8,4,4,1.1f,960.0f/544.0f,-1.0f)==-1);
 assert(memcmp(&occluded,&sentinel,sizeof(occluded))==0);
 cnk_collision_grid_free(&grid);
}
static void original_scenes(const char *vfs_path){
 fufs_file vfs={0};assert(fufs_open(vfs_path,&vfs)==0);
 unsigned scenes=0,games=0;
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *raw=NULL;size_t len=0;
  assert(fufs_extract(&vfs,i,&raw,&len)==0);
  int is_scene=len>=12&&!memcmp(raw+8,"SCEN",4);
  free(raw);
  if(!is_scene)continue;
  cnk_game game={0};
  int rc=cnk_game_load_original_start(&game,&vfs,i,3,2);
  if(rc==0){
   /* This uses the real 3D grid inside game, not a fabricated collision. */
   cnk_camera expected={0};
   assert(cnk_camera_follow_obstructed(&expected,&game.session.racers[0].kart,
           &game.collision,9,5,6,1.2f,960.0f/544.0f,0.4f)==0);
   assert(almost(expected.eye.x,game.session.camera.eye.x));
   assert(almost(expected.eye.z,game.session.camera.eye.z));
   assert(cnk_game_update(&game,(cnk_driver_input){1,0,0},16667)==1);
   assert(cnk_camera_follow_obstructed(&expected,&game.session.racers[0].kart,
           &game.collision,9,5,6,1.2f,960.0f/544.0f,0.4f)==0);
   assert(almost(expected.eye.x,game.session.camera.eye.x));
   assert(almost(expected.eye.y,game.session.camera.eye.y));
   assert(almost(expected.eye.z,game.session.camera.eye.z));
   games++;
  }else{
   /* 2 special scenes are not ordinary races. */
   assert(rc==-8);
  }
  cnk_game_unload(&game);scenes++;
 }
 fufs_close(&vfs);
 assert(scenes==14&&games==12);
 printf("PASS camera occlusion: synthetic wall plus %u/%u source scene game loads, first frame and real step.\n",games,scenes);
}
int main(int argc,char **argv){assert(argc==2);wall_test();original_scenes(argv[1]);return 0;}
