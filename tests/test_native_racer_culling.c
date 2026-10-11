/* Advance 66/67: conservative source-model bounds and VitaGL render pruning.
 * CPU test shim, NOT a VitaSDK/hardware certification. */
#include "cnk_vitagl_objects.h"
#include "cnk_vitagl_game_frame.h"
#include "cnk_track_catalog.h"
#include "vitaGL.h"
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static const float identity[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
static int original_mesh(const fufs_file *vfs,cnk_object_mesh *out){
 for(uint32_t i=0;i<vfs->count;i++){
  unsigned char *data=NULL;size_t len=0;
  if(fufs_extract(vfs,i,&data,&len))return -1;
  int candidate=len>=12&&!memcmp(data,"PIFF",4)&&!memcmp(data+8,"3DOB",4);
  free(data);
  if(candidate)return cnk_object_mesh_from_vfs(vfs,i,out);
 }
 return -1;
}
int main(int argc,char **argv){
 assert(argc==2);
 cnk_camera cam={0};
 assert(!cnk_camera_build(&cam,(cnk_vec3){0,3,-12},(cnk_vec3){0,1,0},
                          1.2f,960.f/544.f,0.1f,300.f));
 cnk_object_mesh test={0};
 test.bbox_min[0]=test.bbox_min[1]=test.bbox_min[2]=-1;
 test.bbox_max[0]=test.bbox_max[1]=test.bbox_max[2]=1;
 float world[16];memcpy(world,identity,sizeof(world));
 assert(cnk_racer_visual_in_frustum(&cam,&test,world)==1);
 world[12]=1000;
 assert(cnk_racer_visual_in_frustum(&cam,&test,world)==0);
 world[12]=0;world[14]=-100;
 assert(cnk_racer_visual_in_frustum(&cam,&test,world)==0);
 world[12]=0;world[14]=0;world[0]=20;
 assert(cnk_racer_visual_in_frustum(&cam,&test,world)==1); /* scaled bounding box */
 world[0]=NAN;assert(cnk_racer_visual_in_frustum(&cam,&test,world)<0);
 world[0]=1;test.bbox_min[1]=NAN;
 assert(cnk_racer_visual_in_frustum(&cam,&test,world)<0);
 /* Synthetic GPU proof: a visible batch and an invisible batch of the
  * SAME actor. Only the visible material submits triangles to vitaGL. */
 cnk_render_vertex verts[6]={{0}};
 cnk_object_batch batches[2]={{0}};
 cnk_object_mesh split={0};
 split.vertices=verts;split.batches=batches;
 split.vertex_count=6;split.triangle_count=2;split.batch_count=2;
 batches[0].first_vertex=0;batches[0].vertex_count=3;
 batches[1].first_vertex=3;batches[1].vertex_count=3;
 for(int a=0;a<3;a++){
   batches[0].bbox_min[a]=1000;batches[0].bbox_max[a]=1001;
   batches[1].bbox_min[a]=-0.5f;batches[1].bbox_max[a]=0.5f;
 }
 split.bbox_min[0]=split.bbox_min[1]=split.bbox_min[2]=-0.5f;
 split.bbox_max[0]=1001;split.bbox_max[1]=1001;split.bbox_max[2]=1001;
 cnk_test_gl_reset(6);
 unsigned material_draws=0;uint32_t material_triangles=0;
 assert(!cnk_vitagl_draw_object_culled(&split,&cam,identity,NULL,NULL,
                                        &material_draws,&material_triangles));
 assert(material_draws==1 && material_triangles==1);
 assert(cnk_test_gl.draw_calls==1 && !cnk_test_gl.invalid);
 batches[1].bbox_min[0]=NAN;
 cnk_test_gl_reset(6);
 assert(cnk_vitagl_draw_object_culled(&split,&cam,identity,NULL,NULL,
                                       &material_draws,&material_triangles)<0);
 assert(cnk_test_gl.draw_calls==0);
 batches[1].bbox_min[0]=-0.5f;
 fufs_file vfs={0};cnk_track_catalog catalog={0};cnk_game game={0};
 cnk_object_mesh original={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(!original_mesh(&vfs,&original));
 assert(!cnk_track_catalog_build(&vfs,&catalog));
 assert(!cnk_game_load_catalog_race(&game,&catalog,0,3,2));
 game.session.camera=cam;
 cnk_racer_visual_attachment karts[CNK_MAX_RACERS]={{0}};
 cnk_racer_visual_attachment drivers[CNK_MAX_RACERS]={{0}};
 for(int i=0;i<3;i++){
  karts[i].mesh=&original;karts[i].local_from_mesh=identity;
  drivers[i].mesh=&original;drivers[i].local_from_mesh=identity;
  assert(!cnk_kart_spawn(&game.session.racers[i].kart,
                       (cnk_vec3){0,0,5},0));
 }
 cnk_test_gl_reset(game.render.vertex_count>original.vertex_count?
                   game.render.vertex_count:original.vertex_count);
 cnk_vitagl_frame_result frame={0};
 assert(!cnk_vitagl_draw_game_frame_with_actors_culled(&game,NULL,NULL,
                                         karts,drivers,&frame));
 assert(frame.racer_models_drawn==3 && frame.driver_models_drawn==3);
 assert(frame.racer_triangles==3*original.triangle_count);
 game.session.racers[2].kart.position.x=10000;
 cnk_test_gl_reset(game.render.vertex_count>original.vertex_count?
                   game.render.vertex_count:original.vertex_count);
 assert(!cnk_vitagl_draw_game_frame_with_actors_culled(&game,NULL,NULL,
                                         karts,drivers,&frame));
 assert(frame.racer_models_drawn==2 && frame.driver_models_drawn==2);
 assert(frame.racer_triangles==2*original.triangle_count);
 assert(frame.driver_triangles==2*original.triangle_count);
 assert(cnk_test_gl.invalid==0);
 /* Preflight must reject corrupt source bounds BEFORE issuing any GL call. */
 cnk_object_mesh broken=original;broken.bbox_min[1]=NAN;
 karts[1].mesh=&broken;
 cnk_test_gl_reset(game.render.vertex_count>original.vertex_count?
                   game.render.vertex_count:original.vertex_count);
 assert(cnk_vitagl_draw_game_frame_with_actors_culled(&game,NULL,NULL,
                                  karts,drivers,&frame)==-9);
 assert(cnk_test_gl.draw_calls==0);
 karts[1].mesh=&original;
 cnk_object_batch *saved_batches=original.batches;
 cnk_object_batch *broken_batches=malloc(sizeof(*broken_batches)*original.batch_count);
 assert(broken_batches);
 memcpy(broken_batches,saved_batches,sizeof(*broken_batches)*original.batch_count);
 broken_batches[0].bbox_min[0]=NAN;
 original.batches=broken_batches;
 cnk_test_gl_reset(game.render.vertex_count>original.vertex_count?
                   game.render.vertex_count:original.vertex_count);
 assert(cnk_vitagl_draw_game_frame_with_actors_culled(&game,NULL,NULL,
                                     karts,drivers,&frame)==-9);
 assert(cnk_test_gl.draw_calls==0);
 original.batches=saved_batches;free(broken_batches);
 cnk_game_unload(&game);cnk_object_mesh_free(&original);fufs_close(&vfs);
 puts("PASS: transformed authentic 3DOB bounds, off-camera kart and driver pruning, atomic GPU preflight");
 return 0;
}
