/* Production vitaGL bridge running against a host-only GPU call recorder.
 * No emulator, no approximation of the original gameplay and no VPK output.
 */
#include "fufs.h"
#include "cnk_scene.h"
#include "cnk_geometry.h"
#include "cnk_render.h"
#include "cnk_model.h"
#include "cnk_object_render.h"
#include "cnk_camera.h"
#include "cnk_vitagl_render.h"
#include "cnk_vitagl_objects.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned scene_lookup(void *p,uint16_t id){
 (*(uint32_t*)p)++;
 /* A non-zero GL handle in the stub tests the bind path, not authenticity.
  * This DOES NOT create a Zeebo PNG-to-material association. */
 return id&1u?9001u:0u;
}
static unsigned object_lookup(void *p,uint32_t material){
 (*(uint32_t*)p)++;
 return material&1u?9002u:0u;
}
static void test_bad_stream(const cnk_camera *camera){
 cnk_render_vertex verts[6]={{0}};
 cnk_render_batch batches[2]={0};
 cnk_render_mesh mesh={0};unsigned calls=999;
 mesh.vertices=verts;mesh.vertex_count=6;mesh.triangle_count=2;
 mesh.batches=batches;mesh.batch_count=2;
 batches[0].first_vertex=0;batches[0].vertex_count=3;
 batches[1].first_vertex=3;batches[1].vertex_count=4;
 cnk_test_gl_reset(6);
 assert(cnk_vitagl_draw_scene(&mesh,camera,NULL,NULL,&calls)==-2);
 assert(!calls && !cnk_test_gl.draw_calls && !cnk_test_gl.matrix_loads);
 batches[1].vertex_count=3;
 batches[1].first_vertex=2; /* non-contiguous; would overlap first draw */
 assert(cnk_vitagl_draw_scene(&mesh,camera,NULL,NULL,&calls)==-2);
 assert(!cnk_test_gl.draw_calls && !cnk_test_gl.matrix_loads);
 batches[1].first_vertex=3;
 cnk_camera bogus=*camera;bogus.projection[1]=NAN;
 assert(cnk_vitagl_draw_scene(&mesh,&bogus,NULL,NULL,&calls)==-3);
 assert(!cnk_test_gl.draw_calls && !cnk_test_gl.matrix_loads);
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 cnk_camera cam={0};
 assert(cnk_camera_build(&cam,(cnk_vec3){0,30,90},(cnk_vec3){0,0,0},
                         1.1f,960.0f/544.0f,0.1f,5000.0f)==0);
 test_bad_stream(&cam);
 fufs_file vfs={0};assert(fufs_open(argv[1],&vfs)==0);
 uint32_t scenes=0,models=0;
 uint64_t scene_tris=0,object_tris=0,scene_draws=0,object_draws=0;
 float ident[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *data=NULL;size_t n=0;
  assert(fufs_extract(&vfs,i,&data,&n)==0);
  if(n>=12 && !memcmp(data,"PIFF",4) && !memcmp(data+8,"SCEN",4)){
   cnk_scene s={0};cnk_geometry geo={0};cnk_render_mesh mesh={0};
   assert(cnk_parse_scene(data,n,&s)==0);
   assert(cnk_geometry_build(&s,1.0f/64.0f,&geo)==0);
   assert(cnk_render_mesh_build(&geo,&mesh)==0);
   cnk_test_gl_reset(mesh.vertex_count);
   unsigned calls=UINT32_MAX;uint32_t lookups=0;
   assert(cnk_vitagl_draw_scene(&mesh,&cam,scene_lookup,&lookups,&calls)==0);
   assert(!cnk_test_gl.invalid && cnk_test_gl.draw_calls==calls);
   assert(cnk_test_gl.drawn_vertices==mesh.vertex_count);
   assert(cnk_test_gl.draw_calls==mesh.batch_count);
   assert(cnk_test_gl.projection_loads==1 && cnk_test_gl.modelview_loads==1);
   assert(cnk_test_gl.matrix_stack_depth==0 && lookups==calls);
   assert(cnk_test_gl.textured_calls+cnk_test_gl.untextured_calls==calls);
   scene_tris+=mesh.triangle_count;scene_draws+=calls;scenes++;
   cnk_render_mesh_free(&mesh);cnk_geometry_free(&geo);cnk_scene_free(&s);
  }else if(n>=12 && !memcmp(data,"PIFF",4) && !memcmp(data+8,"3DOB",4)){
   cnk_model model={0};cnk_object_mesh mesh={0};
   assert(cnk_parse_3dob(data,n,&model)==0);
   assert(cnk_object_mesh_build(&model,&mesh)==0);
   cnk_test_gl_reset(mesh.vertex_count);
   unsigned calls=UINT32_MAX;uint32_t lookups=0;
   assert(cnk_vitagl_draw_object(&mesh,&cam,ident,object_lookup,&lookups,&calls)==0);
   assert(!cnk_test_gl.invalid && cnk_test_gl.draw_calls==calls);
   assert(cnk_test_gl.drawn_vertices==mesh.vertex_count);
   assert(cnk_test_gl.projection_loads==1 && cnk_test_gl.modelview_loads==1);
   assert(cnk_test_gl.matrix_multiplications==1);
   assert(cnk_test_gl.matrix_stack_depth==0 && lookups==calls);
   assert(cnk_test_gl.textured_calls+cnk_test_gl.untextured_calls==calls);
   object_tris+=mesh.triangle_count;object_draws+=calls;models++;
   cnk_object_mesh_free(&mesh);cnk_model_free(&model);
  }
  free(data);
 }
 fufs_close(&vfs);
 printf("GPU bridge: %u SCEN %llu triangles %llu batches; %u 3DOB %llu triangles %llu groups\n",
        scenes,(unsigned long long)scene_tris,(unsigned long long)scene_draws,
        models,(unsigned long long)object_tris,(unsigned long long)object_draws);
 assert(scenes==14&&scene_tris==102756&&models==38&&object_tris==8026);
 assert(scene_draws>=500&&object_draws>=38);
 puts("PASS: native vitaGL draw bridge validated against every decoded source scene/object in host recorder (NOT real Vita GPU testing)");
 return 0;
}
