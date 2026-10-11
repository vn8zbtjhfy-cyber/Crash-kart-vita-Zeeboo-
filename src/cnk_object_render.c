#include "cnk_object_render.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>

void cnk_object_mesh_free(cnk_object_mesh *m){
  if(!m)return;
  free(m->vertices);free(m->source_indices);free(m->batches);
  memset(m,0,sizeof(*m));
}

int cnk_object_mesh_build(const cnk_model *src,cnk_object_mesh *out){
  cnk_object_mesh m={0};
  uint64_t count=0;
  if(!out)return -1;
  memset(out,0,sizeof(*out));
  if(!src||!src->vertices||!src->groups||!src->num_vertices||!src->num_groups ||
     src->num_vertices>65536u||src->num_groups>512u)return -2;
  for(uint32_t i=0;i<src->num_vertices;i++){
    const cnk_vertex *v=&src->vertices[i];
    if(!isfinite(v->x)||!isfinite(v->y)||!isfinite(v->z)||
       !isfinite(v->u)||!isfinite(v->v))return -3;
  }
  for(uint32_t g=0;g<src->num_groups;g++){
    const cnk_triangle_group *p=&src->groups[g];
    if((p->count%3)|| (p->count && !p->indices))return -4;
    count+=p->count;
    if(count>UINT32_MAX || count>SIZE_MAX/sizeof(cnk_render_vertex) ||
       count>SIZE_MAX/sizeof(uint16_t))return -5;
    for(uint32_t j=0;j<p->count;j++)if(p->indices[j]>=src->num_vertices)return -6;
  }
  if(!count)return -7;
  m.vertices=(cnk_render_vertex*)malloc((size_t)count*sizeof(*m.vertices));
  m.source_indices=(uint16_t*)malloc((size_t)count*sizeof(*m.source_indices));
  m.batches=(cnk_object_batch*)calloc(src->num_groups,sizeof(*m.batches));
  if(!m.vertices||!m.source_indices||!m.batches){cnk_object_mesh_free(&m);return -8;}
  for(int k=0;k<3;k++){m.bbox_min[k]=FLT_MAX;m.bbox_max[k]=-FLT_MAX;}
  uint32_t at=0;
  for(uint32_t g=0;g<src->num_groups;g++){
    const cnk_triangle_group *p=&src->groups[g];
    cnk_object_batch *b=&m.batches[g];
    b->material=p->material;b->first_vertex=at;b->vertex_count=p->count;
    for(int k=0;k<3;k++){b->bbox_min[k]=FLT_MAX;b->bbox_max[k]=-FLT_MAX;}
    for(uint32_t j=0;j<p->count;j++){
      const uint16_t index=p->indices[j];
      const cnk_vertex *v=&src->vertices[index];
      cnk_render_vertex *dst=&m.vertices[at];
      dst->x=v->x;dst->y=v->y;dst->z=v->z;
      dst->u=v->u;dst->v=v->v;
      m.source_indices[at++]=index;
      const float pos[3]={v->x,v->y,v->z};
      for(int k=0;k<3;k++){
        if(pos[k]<b->bbox_min[k])b->bbox_min[k]=pos[k];
        if(pos[k]>b->bbox_max[k])b->bbox_max[k]=pos[k];
        if(pos[k]<m.bbox_min[k])m.bbox_min[k]=pos[k];
        if(pos[k]>m.bbox_max[k])m.bbox_max[k]=pos[k];
      }
    }
    /* Preserve empty POLY groups and their materials exactly; mark bounds as
     * zero so a consumer never mistakes FLT_MAX for a real coordinate. */
    if(!b->vertex_count)for(int k=0;k<3;k++)b->bbox_min[k]=b->bbox_max[k]=0;
  }
  m.vertex_count=(uint32_t)count;
  m.triangle_count=(uint32_t)count/3u;
  m.batch_count=src->num_groups;
  *out=m;
  return 0;
}

int cnk_object_mesh_from_vfs(const fufs_file *vfs,uint32_t index,
                             cnk_object_mesh *out){
  if(!out || !vfs || !vfs->data || index>=vfs->count)return -1;
  memset(out,0,sizeof(*out));
  unsigned char *bytes=NULL;size_t count=0;
  if(fufs_extract(vfs,index,&bytes,&count))return -2;
  if(count<12 || memcmp(bytes,"PIFF",4) || memcmp(bytes+8,"3DOB",4)){
    free(bytes);return -3;
  }
  cnk_model m={0};
  int rc=cnk_parse_3dob(bytes,count,&m);
  free(bytes);
  if(rc){cnk_model_free(&m);return -4;}
  rc=cnk_object_mesh_build(&m,out);
  cnk_model_free(&m);
  return rc? -5:0;
}
