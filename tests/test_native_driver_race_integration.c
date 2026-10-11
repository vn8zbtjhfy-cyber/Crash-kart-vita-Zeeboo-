#define _POSIX_C_SOURCE 200809L
#include "cnk_driver_bindings.h"
#include "cnk_verified_characters.h"
#include "cnk_racer_visual.h"
#include "cnk_vitagl_game_frame.h"
#include "cnk_model_lod.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static const float ident[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static void write_bindings(const char *path,int error){
 FILE *f=fopen(path,"wb");assert(f);
 fprintf(f,"# HOST TEST ONLY: invented racer slots and calibration, NOT Zeebo assignments\n");
 const char *chars[]={"crash","coco","ripper_roo"};
 for(unsigned i=0;i<3;i++){
  fprintf(f,"%u\t%s",i,(error==1&&i==2)?"unknown":chars[i]);
  for(unsigned j=0;j<16;j++){
   float v=ident[j];
   if(error==2&&i==2&&j==15)v=0;
   fprintf(f,"\t%.1f",v);
  }
  fputc('\n',f);
 }
 if(error==3){fprintf(f,"0\tcrash");for(unsigned j=0;j<16;j++)fprintf(f,"\t%.1f",ident[j]);fputc('\n',f);}
 assert(fclose(f)==0);
}
int main(int argc,char **argv){
 assert(argc==2);
 char map[]="/tmp/cnk_driver_test_XXXXXX";
 int fd=mkstemp(map);assert(fd>=0);close(fd);
 cnk_driver_bindings b={0};
 write_bindings(map,0);
 assert(cnk_driver_bindings_load(map,&b)==0 && b.count==3);
 assert(b.assigned[0] && b.identity[1]==CNK_CHARACTER_COCO);
 assert(b.identity[2]==CNK_CHARACTER_RIPPER_ROO);
 for(int error=1;error<=3;error++){
  write_bindings(map,error);
  assert(cnk_driver_bindings_load(map,&b)<0 && b.count==0);
 }
 write_bindings(map,0);
 assert(cnk_driver_bindings_load(map,&b)==0);
 assert(cnk_driver_bindings_load("/does/not/exist/cnk.tsv",&b)==-2);
 assert(b.count==0);
 assert(cnk_driver_bindings_load(map,&b)==0);
 fufs_file vfs={0};cnk_png_catalog png={0};cnk_character_pack pack={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&png)==0);
 cnk_test_gl_reset(0);
 assert(cnk_character_pack_load(&vfs,&png,&pack)==0);
 assert(cnk_test_gl.handles_created==3);
 uint8_t kart_slots[CNK_MAX_RACERS]={1,1,1};
 cnk_racer_visual_attachment drivers[CNK_MAX_RACERS]={0};
 assert(cnk_driver_bindings_attachments(&b,&pack,kart_slots,drivers)==0);
 for(unsigned i=0;i<3;i++){
  assert(drivers[i].mesh);
  assert(drivers[i].material_context==&pack.characters[i]);
  assert(drivers[i].resolve_material);
  assert(cnk_racer_visual_attachment_valid(&drivers[i])==0);
 }
 cnk_racer_visual_attachment guard[CNK_MAX_RACERS];
 memcpy(guard,drivers,sizeof(guard));
 kart_slots[1]=0;
 assert(cnk_driver_bindings_attachments(&b,&pack,kart_slots,drivers)<0);
 assert(!memcmp(guard,drivers,sizeof(guard)));
 kart_slots[1]=1;
 /* Source 3DOB family, NEVER historically paired with these drivers. */
 cnk_object_mesh source_kart={0};
 assert(cnk_object_mesh_from_vfs(&vfs,5,&source_kart)==0);
 cnk_racer_visual_attachment karts[CNK_MAX_RACERS]={0};
 for(unsigned i=0;i<3;i++){
  karts[i].mesh=&source_kart;
  karts[i].local_from_mesh=ident;
 }
 cnk_track_catalog tracks={0};cnk_game g={0};
 assert(cnk_track_catalog_build(&vfs,&tracks)==0);
 assert(cnk_game_load_catalog_race(&g,&tracks,0,3,2)==0);
 uint32_t max_vertices=g.render.vertex_count;
 if(source_kart.vertex_count>max_vertices)max_vertices=source_kart.vertex_count;
 for(unsigned i=0;i<3;i++)if(drivers[i].mesh->vertex_count>max_vertices)
  max_vertices=drivers[i].mesh->vertex_count;
 cnk_test_gl.vertex_limit=max_vertices;
 cnk_vitagl_frame_result frame={0};
 assert(cnk_vitagl_draw_game_frame_with_actors_culled(&g,NULL,NULL,karts,drivers,&frame)==0);
 assert(!cnk_test_gl.invalid);
 /* After authentic STRT staging, trailing racers may be off camera.
  * Check the render against frustum visibility rather than demanding that
  * all synthetic character/vehicle bindings be drawn simultaneously. */
 unsigned seen[2]={0};
 unsigned draw_order[2][3]={{0}};
 const cnk_racer_visual_attachment *all[2]={karts,drivers};
 for(unsigned layer=0;layer<2;layer++)for(unsigned i=0;i<3;i++){
  float world[16]={0};
  assert(cnk_racer_visual_matrix(&g.session.racers[i].kart,
        all[layer][i].local_from_mesh,world)==0);
  int on_screen=cnk_racer_visual_in_frustum(&g.session.camera,
        all[layer][i].mesh,world);
  assert(on_screen>=0);
  if(on_screen)draw_order[layer][seen[layer]++]=i;
 }
 assert(frame.racer_models_drawn==seen[0]);
 assert(frame.driver_models_drawn==seen[1]);
 assert(frame.racer_triangles<=source_kart.triangle_count*seen[0]);
 assert(frame.driver_triangles<=drivers[0].mesh->triangle_count+
        drivers[1].mesh->triangle_count+drivers[2].mesh->triangle_count);
 assert(seen[0]>0 && seen[1]>0);
 assert(frame.racer_draw_calls>0 && frame.driver_draw_calls>0);
 assert(cnk_test_gl.textured_calls>=seen[1]);
 assert(cnk_test_gl.matrix_multiplications==seen[0]+seen[1]);
 assert(cnk_test_gl.matrix_stack_depth==0);
 /* Karts and their drivers, if both in view, must share a world origin. */
 for(unsigned i=0;i<3;i++){
  int a=-1,b=-1;
  for(unsigned j=0;j<seen[0];j++)if(draw_order[0][j]==i)a=(int)j;
  for(unsigned j=0;j<seen[1];j++)if(draw_order[1][j]==i)b=(int)j;
  if(a<0||b<0)continue;
  const float *kart_m=cnk_test_gl.recorded_model_matrices+(size_t)a*16;
  const float *driver_m=cnk_test_gl.recorded_model_matrices+
                         (seen[0]+(unsigned)b)*16;
  assert(fabsf(kart_m[12]-driver_m[12])<1e-3f);
  assert(fabsf(kart_m[13]-driver_m[13])<1e-3f);
  assert(fabsf(kart_m[14]-driver_m[14])<1e-3f);
 }
 /* Driver validation MUST happen before even the scene is drawn. */
 float invalid[16];memcpy(invalid,ident,sizeof(invalid));invalid[8]=NAN;
 drivers[2].local_from_mesh=invalid;
 unsigned before=cnk_test_gl.draw_calls;
 assert(cnk_vitagl_draw_game_frame_with_actors_culled(&g,NULL,NULL,karts,drivers,&frame)==-4);
 assert(cnk_test_gl.draw_calls==before);
 drivers[2].local_from_mesh=b.calibration[2];
 assert(cnk_game_update(&g,(cnk_driver_input){1,0,0},16667)==1);
 assert(cnk_vitagl_draw_game_frame_with_actors_culled(&g,NULL,NULL,karts,drivers,&frame)==0);
 cnk_game_unload(&g);
 cnk_object_mesh_free(&source_kart);
 cnk_character_pack_free(&pack);
 assert(cnk_test_gl.handles_deleted==cnk_test_gl.handles_created);
 cnk_png_catalog_free(&png);
 fufs_close(&vfs);unlink(map);
 puts("PASS: source 3DOB kart + 3 textured original character models on moving race slots; all pairings are HOST TEST ONLY, no original kart/driver identities inferred");
 return 0;
}
