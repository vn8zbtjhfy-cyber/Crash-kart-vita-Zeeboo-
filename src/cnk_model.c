#include "cnk_model.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_VERTICES 250000u
#define MAX_GROUPS 512u
#define MAX_INDICES 3000000u
static uint32_t rd32(const unsigned char *p){
 return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static uint16_t rd16(const unsigned char *p){return (uint16_t)(p[0] | ((uint16_t)p[1]<<8));}
static float fixed16(const unsigned char *p){
 uint32_t raw=rd32(p);
 int64_t signed_value=raw >= 0x80000000u ? (int64_t)raw - 4294967296LL : (int64_t)raw;
 return (float)(signed_value / 65536.0);
}
void cnk_model_free(cnk_model *m){
 uint32_t i;
 if(!m)return;
 for(i=0;i<m->num_groups;i++) free(m->groups[i].indices);
 free(m->groups);free(m->vertices);memset(m,0,sizeof(*m));
}
int cnk_parse_3dob(const unsigned char *data,size_t size,cnk_model *out){
 size_t pos;const unsigned char *mesh=NULL,*uv=NULL;uint32_t vertex_count=0,uv_count=0,group_count=0,k=0;
 cnk_model result;
 if(!out) return -1;
 memset(out,0,sizeof(*out)); memset(&result,0,sizeof(result));
 if(!data || size<12 || memcmp(data,"PIFF",4) || rd32(data+4)!=size-8 || memcmp(data+8,"3DOB",4))return -2;
 for(pos=12;pos<size;){
  size_t n;const unsigned char *p;
  if(size-pos<8)return -3;
  n=rd32(data+pos+4);
  if(n>size-pos-8)return -4;
  p=data+pos+8;
  if(!memcmp(data+pos,"MESH",4)){
   if(mesh || (n%12)!=0 || (n/12)>MAX_VERTICES)return -5;
   mesh=p;vertex_count=(uint32_t)(n/12);
  } else if(!memcmp(data+pos,"TXUV",4)){
   if(uv || (n%8)!=0 || (n/8)>MAX_VERTICES)return -6;
   uv=p;uv_count=(uint32_t)(n/8);
  } else if(!memcmp(data+pos,"POLY",4)){
   uint32_t count,type,primitive;
   size_t width;
   if(n<16 || ++group_count>MAX_GROUPS)return -7;
   primitive=rd32(p);count=rd32(p+4);type=rd32(p+12);
   width= type==0x1401u?1u: type==0x1403u?2u:0u;
   if(primitive!=4u || !width || count>MAX_INDICES || (count%3)!=0 || 16u+(size_t)count*width!=n)return -8;
  }
  pos+=8+n;
 }
 if(pos!=size || !mesh || !uv || !vertex_count || vertex_count!=uv_count || !group_count)return -9;
 result.num_vertices=vertex_count;
 result.vertices=(cnk_vertex*)calloc(vertex_count,sizeof(cnk_vertex));
 result.groups=(cnk_triangle_group*)calloc(group_count,sizeof(cnk_triangle_group));
 if(!result.vertices || !result.groups){cnk_model_free(&result);return -10;}
 result.num_groups=group_count;
 for(uint32_t i=0;i<vertex_count;i++){
  cnk_vertex *v=&result.vertices[i];
  v->x=fixed16(mesh+i*12);v->y=fixed16(mesh+i*12+4);v->z=fixed16(mesh+i*12+8);
  v->u=fixed16(uv+i*8);v->v=fixed16(uv+i*8+4);
 }
 for(pos=12;pos<size;){
  uint32_t n=rd32(data+pos+4);
  if(!memcmp(data+pos,"POLY",4)){
   const unsigned char *p=data+pos+8;
   uint32_t count=rd32(p+4),type=rd32(p+12);
   cnk_triangle_group *g=&result.groups[k++];
   g->indices=(uint16_t*)malloc((size_t)count*sizeof(uint16_t));
   if(!g->indices){cnk_model_free(&result);return -11;}
   g->count=count;g->material=rd32(p+8);g->original_gl_index_type=type;
   for(uint32_t i=0;i<count;i++){
    uint16_t idx=type==0x1401u?p[16+i]:rd16(p+16+2*i);
    if(idx>=vertex_count){cnk_model_free(&result);return -12;}
    g->indices[i]=idx;
   }
  }
  pos+=8+(size_t)n;
 }
 *out=result;
 return 0;
}
