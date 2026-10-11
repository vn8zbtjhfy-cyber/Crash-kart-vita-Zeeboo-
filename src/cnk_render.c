#include "cnk_render.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

typedef struct {uint16_t material,texture;} render_key;
static int cmp_key(const void *a,const void *b){
 const render_key *x=(const render_key*)a,*y=(const render_key*)b;
 if(x->material!=y->material)return x->material<y->material?-1:1;
 return x->texture==y->texture?0:(x->texture<y->texture?-1:1);
}
static int find_key(const render_key *keys,uint32_t n,render_key key){
 uint32_t lo=0,hi=n;
 while(lo<hi){uint32_t mid=lo+(hi-lo)/2;int c=cmp_key(&keys[mid],&key);
 if(c<0)lo=mid+1;else hi=mid;}
 return lo<n && cmp_key(&keys[lo],&key)==0?(int)lo:-1;
}
void cnk_render_mesh_free(cnk_render_mesh *m){
 if(!m)return;free(m->vertices);free(m->source_triangles);free(m->batches);
 memset(m,0,sizeof(*m));
}
int cnk_render_mesh_build(const cnk_geometry *src,cnk_render_mesh *out){
 cnk_render_mesh tmp={0};render_key *keys=NULL;uint32_t *counts=NULL,*next=NULL;
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!src || !src->triangles || !src->triangle_count ||
    src->triangle_count>UINT32_MAX/3u || src->triangle_count>1000000u)return -2;
 uint32_t n=src->triangle_count;
 if((uint64_t)n*3u > SIZE_MAX/sizeof(cnk_render_vertex) ||
    n>SIZE_MAX/sizeof(render_key))return -3;
 keys=(render_key*)malloc(n*sizeof(*keys));
 if(!keys)return -4;
 for(uint32_t i=0;i<n;i++){
  const cnk_geo_triangle *t=&src->triangles[i];
  keys[i]=(render_key){t->material,t->texture};
  for(int k=0;k<3;k++)if(!isfinite(t->v[k].x)||!isfinite(t->v[k].y)||!isfinite(t->v[k].z)){
   free(keys);return -5;
  }
 }
 qsort(keys,n,sizeof(*keys),cmp_key);
 uint32_t unique=0;
 for(uint32_t i=0;i<n;i++)if(!i || cmp_key(&keys[i],&keys[unique-1]))keys[unique++]=keys[i];
 if(unique>65536u){free(keys);return -3;}
 counts=(uint32_t*)calloc(unique,sizeof(*counts));
 next=(uint32_t*)calloc(unique,sizeof(*next));
 tmp.vertices=(cnk_render_vertex*)malloc((size_t)n*3*sizeof(*tmp.vertices));
 tmp.source_triangles=(uint32_t*)malloc((size_t)n*sizeof(*tmp.source_triangles));
 tmp.batches=(cnk_render_batch*)calloc(unique,sizeof(*tmp.batches));
 if(!counts||!next||!tmp.vertices||!tmp.source_triangles||!tmp.batches)goto fail;
 for(uint32_t i=0;i<n;i++){
  const cnk_geo_triangle *t=&src->triangles[i];
  int g=find_key(keys,unique,(render_key){t->material,t->texture});
  if(g<0)goto fail;
  counts[g]++;
 }
 uint32_t at=0;
 for(uint32_t g=0;g<unique;g++){
  cnk_render_batch *b=&tmp.batches[g];
  b->material=keys[g].material;b->texture=keys[g].texture;
  b->first_vertex=at*3;b->vertex_count=counts[g]*3;
  for(int c=0;c<3;c++)b->bbox_min[c]=FLT_MAX,b->bbox_max[c]=-FLT_MAX;
  next[g]=at;
  at+=counts[g];
 }
 if(at!=n)goto fail;
 for(uint32_t i=0;i<n;i++){
  const cnk_geo_triangle *t=&src->triangles[i];
  int gi=find_key(keys,unique,(render_key){t->material,t->texture});
  if(gi<0)goto fail;
  uint32_t index=next[gi]++;
  cnk_render_batch *b=&tmp.batches[gi];
  tmp.source_triangles[index]=i;
  for(int v=0;v<3;v++){
   const cnk_geo_vertex *p=&t->v[v];
   cnk_render_vertex *q=&tmp.vertices[index*3u+(uint32_t)v];
   q->x=p->x;q->y=p->y;q->z=p->z;
   if(p->uv_valid){q->u=(float)p->u;q->v=(float)p->v;}
   else {q->u=q->v=0.0f;tmp.skipped_invalid_uv++;}
   float xyz[3]={q->x,q->y,q->z};
   for(int c=0;c<3;c++){
    if(xyz[c]<b->bbox_min[c])b->bbox_min[c]=xyz[c];
    if(xyz[c]>b->bbox_max[c])b->bbox_max[c]=xyz[c];
   }
  }
 }
 free(keys);free(counts);free(next);
 tmp.triangle_count=n;tmp.vertex_count=n*3;tmp.batch_count=unique;
 *out=tmp;return 0;
fail:
 free(keys);free(counts);free(next);cnk_render_mesh_free(&tmp);return -4;
}
