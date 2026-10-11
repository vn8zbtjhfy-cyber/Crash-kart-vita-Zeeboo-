/* Conservative per-material batch frustum culling test. Vita GL API calls
 * are recorded in PC; this does NOT test performance on physical hardware. */
#include "cnk_frustum.h"
#include "cnk_vitagl_render.h"
#include "cnk_scene.h"
#include "cnk_geometry.h"
#include "fufs.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void create_batch(cnk_render_batch *b,unsigned first,float x,float y,float z){
 memset(b,0,sizeof(*b));
 b->first_vertex=first;b->vertex_count=3;
 b->bbox_min[0]=x-.5f;b->bbox_max[0]=x+.5f;
 b->bbox_min[1]=y-.5f;b->bbox_max[1]=y+.5f;
 b->bbox_min[2]=z-.5f;b->bbox_max[2]=z+.5f;
}
static void synthetic_test(void){
 cnk_camera cam={0};
 assert(cnk_camera_build(&cam,(cnk_vec3){0,0,10},(cnk_vec3){0,0,0},
                         1.0f,1.0f,0.2f,100.0f)==0);
 cnk_render_vertex vertices[9]={{0}};
 cnk_render_batch batch[3];
 create_batch(&batch[0],0,0,0,0); /* centered ahead */
 create_batch(&batch[1],3,0,0,30); /* behind viewer */
 create_batch(&batch[2],6,200,0,0); /* fully outside right plane */
 cnk_render_mesh mesh={0};mesh.vertices=vertices;mesh.batches=batch;
 mesh.vertex_count=9;mesh.triangle_count=3;mesh.batch_count=3;
 assert(cnk_batch_in_frustum(&cam,&batch[0])==1);
 assert(cnk_batch_in_frustum(&cam,&batch[1])==0);
 assert(cnk_batch_in_frustum(&cam,&batch[2])==0);
 cnk_test_gl_reset(9);
 unsigned draw=999;uint32_t triangles=999;
 assert(cnk_vitagl_draw_scene_culled(&mesh,&cam,NULL,NULL,&draw,&triangles)==0);
 assert(draw==1&&triangles==1&&cnk_test_gl.draw_calls==1&&
        cnk_test_gl.drawn_vertices==3&&!cnk_test_gl.invalid);
 cnk_test_gl_reset(9);
 assert(cnk_vitagl_draw_scene(&mesh,&cam,NULL,NULL,&draw)==0);
 assert(draw==3&&cnk_test_gl.drawn_vertices==9&&!cnk_test_gl.invalid);
 /* Do not accidentally cull geometry intersecting a near/far edge. */
 create_batch(&batch[0],0,0,0,9.75f);
 assert(cnk_batch_in_frustum(&cam,&batch[0])==1);
 /* Invalid bounds reject BEFORE touching GPU. */
 batch[1].bbox_min[0]=NAN;
 cnk_test_gl_reset(9);
 assert(cnk_vitagl_draw_scene_culled(&mesh,&cam,NULL,NULL,&draw,&triangles)==-4);
 assert(cnk_test_gl.draw_calls==0&&cnk_test_gl.matrix_loads==0);
 assert(cnk_batch_in_frustum(&cam,NULL)==-1);
}
static void source_scene_test(const char *vfs_path){
 fufs_file f={0};assert(fufs_open(vfs_path,&f)==0);
 uint32_t scenes=0;uint64_t checked=0,drawn=0,hidden=0;
 cnk_camera cam={0};
 assert(cnk_camera_build(&cam,(cnk_vec3){5,12,35},(cnk_vec3){0,0,0},
        1.2f,960.f/544.f,.1f,8000.f)==0);
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *p=NULL;size_t n=0;
  assert(fufs_extract(&f,i,&p,&n)==0);
  if(n>=12&&!memcmp(p,"PIFF",4)&&!memcmp(p+8,"SCEN",4)){
   cnk_scene scene={0};cnk_geometry geometry={0};cnk_render_mesh mesh={0};
   assert(cnk_parse_scene(p,n,&scene)==0);
   assert(cnk_geometry_build(&scene,1.f/64.f,&geometry)==0);
   assert(cnk_render_mesh_build(&geometry,&mesh)==0);
   uint32_t predicted=0,tri=0;unsigned predicted_calls=0,calls=0;
   for(uint32_t j=0;j<mesh.batch_count;j++){
    int visible=cnk_batch_in_frustum(&cam,&mesh.batches[j]);
    assert(visible==0||visible==1);
    if(visible){predicted+=mesh.batches[j].vertex_count/3;predicted_calls++;}
    checked++;
   }
   cnk_test_gl_reset(mesh.vertex_count);
   assert(cnk_vitagl_draw_scene_culled(&mesh,&cam,NULL,NULL,&calls,&tri)==0);
   assert(!cnk_test_gl.invalid&&calls==predicted_calls&&tri==predicted&&
          cnk_test_gl.drawn_vertices==tri*3u);
   drawn+=tri;hidden+=mesh.triangle_count-tri;scenes++;
   cnk_render_mesh_free(&mesh);cnk_geometry_free(&geometry);cnk_scene_free(&scene);
  }
  free(p);
 }
 assert(scenes==14&&checked>500&&drawn+hidden==102756u);
 fufs_close(&f);
 printf("Frustum: %u original scenes, %llu batches, %llu triangles retained, %llu safely culled at a reference camera\n",
 scenes,(unsigned long long)checked,(unsigned long long)drawn,(unsigned long long)hidden);
}
int main(int argc,char **argv){
 assert(argc==2);
 synthetic_test();source_scene_test(argv[1]);
 puts("PASS: conservative native GPU batch culling (host GL recorder; no Vita device test)");
 return 0;
}
