#include "cnk_scene.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_SCENE_ELEMENTS 300000u
#define MAX_CHUNKS 8192u
#define MAX_LINES 128u
#define MAX_TEXTURES 1024u
#define MAX_POINTS 50000u

static uint16_t r16(const unsigned char *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static uint32_t r32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int16_t s16(const unsigned char *p){uint16_t n=r16(p);return n<32768?(int16_t)n:(int16_t)((int32_t)n-65536);}
static float fixed(const unsigned char *p){uint32_t n=r32(p);int64_t a=n<0x80000000u?n:(int64_t)n-4294967296LL;return (float)(a/65536.0);}
static void copy_id(char id[5],const unsigned char *p){memcpy(id,p,4);id[4]=0;}
static int alloc(void **dst,size_t n,size_t bytes){
 if(!n || n > (size_t)-1 / bytes)return -1;
 *dst=calloc(n,bytes);return *dst?0:-1;
}
void cnk_scene_free(cnk_scene *s){
 if(!s)return;
 if(s->chunks){for(uint32_t i=0;i<s->chunk_count;i++)free(s->chunks[i].indices);}
 if(s->lines){for(uint32_t i=0;i<s->line_count;i++)free(s->lines[i].points);}
 if(s->texture_names){for(uint32_t i=0;i<s->texture_count;i++)free(s->texture_names[i]);}
 free(s->vertices);free(s->uvs);free(s->colors);free(s->materials);
 free(s->texture_names);free(s->chunks);free(s->voffsets);free(s->lines);free(s->positions);
 memset(s,0,sizeof(*s));
}
const cnk_sline *cnk_scene_line(const cnk_scene *s,const char *id){
 if(!s||!id)return NULL;
 for(uint32_t i=0;i<s->line_count;i++)if(!strncmp(s->lines[i].id,id,4))return &s->lines[i];
 return NULL;
}
int cnk_parse_scene(const unsigned char *data,size_t size,cnk_scene *out){
 /* For all errors, free everything. No caller-owned input retained. */
 cnk_scene a;const unsigned char *ptr[6]={0};size_t lens[6]={0};
 static const char names[6][5]={"VERT","TCOR","COLR","MATL","CHNK","INDS"};
 const unsigned char *voff=NULL,*line=NULL,*posi=NULL,*txtr=NULL;
 size_t vlen=0,llen=0,plen=0,tlen=0;
 int err=-1;
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 memset(&a,0,sizeof(a));
 if(!data || size<12 || memcmp(data,"PIFF",4) || (size-8)!=(size_t)r32(data+4) || memcmp(data+8,"SCEN",4))return -2;
 for(size_t at=12;at<size;){
  if(size-at<8){err=-3;goto fail;}
  size_t n=r32(data+at+4);
  if(n>size-at-8){err=-4;goto fail;}
  const unsigned char *p=data+at+8;
  for(int j=0;j<6;j++)if(!memcmp(data+at,names[j],4)){
   if(ptr[j]){err=-5;goto fail;}
   ptr[j]=p;lens[j]=n;
  }
  if(!memcmp(data+at,"VOFF",4)){if(voff){err=-5;goto fail;}voff=p;vlen=n;}
  if(!memcmp(data+at,"LINE",4)){if(line){err=-5;goto fail;}line=p;llen=n;}
  if(!memcmp(data+at,"POSI",4)){if(posi){err=-5;goto fail;}posi=p;plen=n;}
  if(!memcmp(data+at,"TXTR",4)){if(txtr){err=-5;goto fail;}txtr=p;tlen=n;}
  at+=n+8;
 }
 for(int j=0;j<6;j++)if(!ptr[j]||lens[j]<4){err=-6;goto fail;}
 if(!voff || !line || !posi || !txtr || vlen<4 || llen<4 || plen<4 || tlen<4){err=-7;goto fail;}
 if(lens[2]<8){err=-9;goto fail;}
 a.vertex_count=r32(ptr[0]);a.uv_count=r32(ptr[1]);
 a.color_count=r32(ptr[2]+4);a.material_count=r32(ptr[3]);
 a.chunk_count=r32(ptr[4]);a.voffset_count=r32(voff);
 a.position_count=r32(posi);a.texture_count=r32(txtr);a.line_count=r16(line);
 if(r32(ptr[2])!=1 ||
    a.vertex_count>MAX_SCENE_ELEMENTS || !a.vertex_count ||
    a.uv_count>MAX_SCENE_ELEMENTS || !a.uv_count ||
    a.color_count>MAX_SCENE_ELEMENTS || !a.color_count ||
    !a.material_count || a.material_count>MAX_TEXTURES ||
    !a.texture_count || a.texture_count>MAX_TEXTURES ||
    !a.chunk_count || a.chunk_count>MAX_CHUNKS ||
    !a.voffset_count || a.voffset_count>MAX_CHUNKS ||
    !a.line_count || a.line_count>MAX_LINES ||
    a.position_count>MAX_LINES){err=-8;goto fail;}
 if(lens[0]!=4+(size_t)a.vertex_count*6 || lens[1]!=4+(size_t)a.uv_count*4 ||
    lens[2]!=8+(size_t)a.color_count*2 || lens[3]!=4+(size_t)a.material_count*8 ||
    lens[4]!=4+(size_t)a.chunk_count*14 ||
    vlen!=4+(size_t)a.voffset_count*20 || plen!=4+(size_t)a.position_count*32 ||
    r32(ptr[5])!=lens[5]-4 || r16(line+2)!=llen-4){err=-9;goto fail;}
 if(alloc((void**)&a.vertices,a.vertex_count,sizeof(*a.vertices)) ||
    alloc((void**)&a.uvs,a.uv_count,sizeof(*a.uvs)) ||
    alloc((void**)&a.colors,a.color_count,sizeof(*a.colors)) ||
    alloc((void**)&a.materials,a.material_count,sizeof(*a.materials)) ||
    alloc((void**)&a.chunks,a.chunk_count,sizeof(*a.chunks)) ||
    alloc((void**)&a.voffsets,a.voffset_count,sizeof(*a.voffsets)) ||
    alloc((void**)&a.lines,a.line_count,sizeof(*a.lines)) ||
    alloc((void**)&a.texture_names,a.texture_count,sizeof(*a.texture_names))){err=-10;goto fail;}
 if(a.position_count && alloc((void**)&a.positions,a.position_count,sizeof(*a.positions))){err=-10;goto fail;}
 for(uint32_t i=0;i<a.vertex_count;i++){
  const unsigned char *p=ptr[0]+4+i*6;
  a.vertices[i]=(cnk_svertex){s16(p),s16(p+2),s16(p+4)};
 }
 for(uint32_t i=0;i<a.uv_count;i++){
  const unsigned char *p=ptr[1]+4+i*4;
  a.uvs[i]=(cnk_suv){s16(p),s16(p+2)};
 }
 for(uint32_t i=0;i<a.color_count;i++)a.colors[i]=r16(ptr[2]+8+i*2);
 for(uint32_t i=0;i<a.material_count;i++){
  const unsigned char *p=ptr[3]+4+i*8;
  a.materials[i]=(cnk_smaterial){r16(p),r16(p+2),r16(p+4),r16(p+6)};
 }
 /* Short, length-prefixed texture paths in the original SCEN resource. */
 size_t at=4;
 for(uint32_t i=0;i<a.texture_count;i++){
  if(at>=tlen){err=-11;goto fail;}
  size_t n=txtr[at++];if(n>tlen-at || n>250){err=-11;goto fail;}
  a.texture_names[i]=(char*)calloc(n+1,1);
  if(!a.texture_names[i]){err=-10;goto fail;}
  memcpy(a.texture_names[i],txtr+at,n);at+=n;
 }
 if(at!=tlen){err=-11;goto fail;}
 /* CHNK contains per-material triangles; indices within INDS are 8-bit
  * except index_width=2, which is 16-bit. Index offset is counted in words. */
 for(uint32_t i=0;i<a.chunk_count;i++){
  const unsigned char *p=ptr[4]+4+i*14;
  cnk_schunk *c=&a.chunks[i];
  c->material=r16(p);c->vertex_base=r16(p+2);c->uv_base=r16(p+4);
  c->color_base=r16(p+6);c->index_width=r16(p+8);
  c->index_count=r16(p+10);c->index_word_offset=r16(p+12);
  size_t offset=4+(size_t)c->index_word_offset*2;
  size_t bytes=(size_t)c->index_width*c->index_count;
  if(c->material>=a.material_count || c->vertex_base>=a.vertex_count ||
     (c->index_width!=1 && c->index_width!=2) ||
     !c->index_count || c->index_count%3 ||
     offset>lens[5] || bytes>lens[5]-offset){err=-12;goto fail;}
  if(alloc((void**)&c->indices,c->index_count,sizeof(uint16_t))){err=-10;goto fail;}
  const unsigned char *ip=ptr[5]+offset;
  for(uint32_t j=0;j<c->index_count;j++){
   uint16_t index=c->index_width==1?ip[j]:r16(ip+j*2);
   if((uint32_t)c->vertex_base+index>=a.vertex_count){err=-13;goto fail;}
   c->indices[j]=index;
  }
 }
 for(uint32_t i=0;i<a.voffset_count;i++){
  const unsigned char *p=voff+4+i*20;
  cnk_voffset *v=&a.voffsets[i];
  v->origin=(cnk_vec3){fixed(p),fixed(p+4),fixed(p+8)};
  v->start=r16(p+12);v->count=r16(p+14);v->metadata=r32(p+16);
  if((uint32_t)v->start+v->count>a.vertex_count){err=-14;goto fail;}
 }
 /* Original racing splines (DRV0/1/2), lap checkpoints (LAP*) and cameras. */
 at=4;
 for(uint32_t i=0;i<a.line_count;i++){
  if(at>llen || llen-at<8){err=-15;goto fail;}
  cnk_sline *path=&a.lines[i];copy_id(path->id,line+at);
  path->count=r32(line+at+4);at+=8;
  if(!path->count || path->count>MAX_POINTS || (size_t)path->count>(llen-at)/12){err=-16;goto fail;}
  if(alloc((void**)&path->points,path->count,sizeof(*path->points))){err=-10;goto fail;}
  for(uint32_t j=0;j<path->count;j++){
   const unsigned char *p=line+at+j*12;
   path->points[j]=(cnk_vec3){fixed(p),fixed(p+4),fixed(p+8)};
  }
  at+=(size_t)path->count*12;
 }
 if(at!=llen){err=-17;goto fail;}
 for(uint32_t i=0;i<a.position_count;i++){
  const unsigned char *p=posi+4+i*32;
  cnk_sposition *v=&a.positions[i];copy_id(v->id,p);
  v->kind=r32(p+4);v->point=(cnk_vec3){fixed(p+8),fixed(p+12),fixed(p+16)};
  for(int j=0;j<3;j++)v->flags[j]=(int32_t)r32(p+20+j*4);
 }
 *out=a;
 return 0;
fail:
 cnk_scene_free(&a);
 return err;
}

int cnk_scene_start_pose(const cnk_scene *s,cnk_vec3 *point,float *heading){
 if(!s||!point||!heading)return -1;
 for(uint32_t i=0;i<s->position_count;i++){
  const cnk_sposition *p=&s->positions[i];
  if(strcmp(p->id,"STRT"))continue;
  float dx=(float)p->flags[0]/65536.0f;
  float dz=(float)p->flags[2]/65536.0f;
  float norm=hypotf(dx,dz);
  if(!isfinite(p->point.x)||!isfinite(p->point.y)||!isfinite(p->point.z)||
     !isfinite(norm)||norm<0.90f||norm>1.10f)return -2;
  *point=p->point;
  *heading=atan2f(dx,dz);
  return 0;
 }
 return -3;
}
