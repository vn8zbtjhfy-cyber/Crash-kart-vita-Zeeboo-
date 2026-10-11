#include "fufs.h"
#include "cnk_object_render.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

static int closef(float a,float b){return fabsf(a-b)<0.00001f;}
static void negative(void){
 cnk_vertex v[3]={{0,0,0,0,0},{1,0,0,1,0},{0,1,0,0,1}};
 uint16_t indices[3]={0,1,2};
 cnk_triangle_group group={indices,3,90001,0x1403};
 cnk_model model={v,3,&group,1};
 cnk_object_mesh mesh={0};
 assert(!cnk_object_mesh_build(&model,&mesh));
 assert(mesh.batch_count==1 && mesh.vertex_count==3);
 assert(mesh.batches[0].material==90001u);
 assert(mesh.source_indices[0]==0 && mesh.source_indices[1]==1 && mesh.source_indices[2]==2);
 assert(closef(mesh.bbox_max[0],1) && closef(mesh.bbox_max[1],1));
 cnk_object_mesh_free(&mesh);
 indices[2]=3;assert(cnk_object_mesh_build(&model,&mesh)==-6);
 indices[2]=2;v[1].u=NAN;assert(cnk_object_mesh_build(&model,&mesh)==-3);
 v[1].u=1;group.count=2;assert(cnk_object_mesh_build(&model,&mesh)==-4);
 group.count=3;assert(cnk_object_mesh_build(NULL,&mesh)==-2);
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 negative();
 fufs_file f={0};assert(fufs_open(argv[1],&f)==0);
 uint32_t objects=0;uint64_t triangles=0,drawcalls=0,expanded=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *blob=NULL;size_t size=0;
  assert(fufs_extract(&f,i,&blob,&size)==0);
  if(size>=12 && memcmp(blob,"PIFF",4)==0 && memcmp(blob+8,"3DOB",4)==0){
   cnk_model model={0};cnk_object_mesh mesh={0};
   assert(cnk_parse_3dob(blob,size,&model)==0);
   assert(cnk_object_mesh_build(&model,&mesh)==0);
   assert(mesh.batch_count==model.num_groups);
   uint32_t cursor=0;
   for(uint32_t g=0;g<model.num_groups;g++){
    const cnk_triangle_group *group=&model.groups[g];
    const cnk_object_batch *batch=&mesh.batches[g];
    assert(batch->material==group->material);
    assert(batch->first_vertex==cursor&&batch->vertex_count==group->count);
    for(uint32_t j=0;j<group->count;j++){
     const uint32_t p=cursor+j;
     const uint16_t index=group->indices[j];
     const cnk_vertex *src=&model.vertices[index];
     const cnk_render_vertex *dst=&mesh.vertices[p];
     assert(mesh.source_indices[p]==index);
     assert(closef(src->x,dst->x)&&closef(src->y,dst->y)&&closef(src->z,dst->z));
     assert(closef(src->u,dst->u)&&closef(src->v,dst->v));
    }
    cursor+=group->count;
    if(group->count)drawcalls++;
   }
   assert(cursor==mesh.vertex_count && mesh.triangle_count==mesh.vertex_count/3);
   triangles+=mesh.triangle_count;expanded+=mesh.vertex_count;objects++;
   cnk_object_mesh_free(&mesh);cnk_model_free(&model);
  }
  free(blob);
 }
 fufs_close(&f);
 printf("3DOB=%u triangles=%llu expanded_vertices=%llu nonempty_groups=%llu\n",objects,
  (unsigned long long)triangles,(unsigned long long)expanded,(unsigned long long)drawcalls);
 assert(objects==38&&triangles==8026&&expanded==24078);
 puts("PASS: all native 3DOB vertices/UV/material groups preserved");
 return 0;
}
