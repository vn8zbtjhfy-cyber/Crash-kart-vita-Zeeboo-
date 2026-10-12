#include "cnk_collision.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#define MAX_GRID_REFS 3000000u
#define MAX_GRID_CELLS 4096u

static uint32_t grid_cell(float value,float min,float step,uint32_t size){
 float coordinate=(value-min)/step;
 if(coordinate<=0.0f)return 0;
 if(coordinate>=(float)size)return size-1;
 return (uint32_t)coordinate;
}
void cnk_collision_grid_free(cnk_collision_grid *g){
 if(g){free(g->offsets);free(g->indices);memset(g,0,sizeof(*g));}
}
static void get_range(const cnk_collision_grid *g,const cnk_geo_triangle *t,
                      uint32_t *xa,uint32_t *xb,uint32_t *za,uint32_t *zb){
 float minx=t->v[0].x,maxx=minx,minz=t->v[0].z,maxz=minz;
 for(int k=1;k<3;k++){
   if(t->v[k].x<minx)minx=t->v[k].x;
   if(t->v[k].x>maxx)maxx=t->v[k].x;
   if(t->v[k].z<minz)minz=t->v[k].z;
   if(t->v[k].z>maxz)maxz=t->v[k].z;
 }
 *xa=grid_cell(minx,g->minx,g->dx,g->cols);
 *xb=grid_cell(maxx,g->minx,g->dx,g->cols);
 *za=grid_cell(minz,g->minz,g->dz,g->rows);
 *zb=grid_cell(maxz,g->minz,g->dz,g->rows);
}
int cnk_collision_grid_build(const cnk_geometry *geom,uint32_t cols,uint32_t rows,cnk_collision_grid *out){
 cnk_collision_grid g={0};
 uint32_t *counts=NULL,*current=NULL;
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!geom || !geom->triangles || !geom->triangle_count || !cols || !rows ||
    cols>MAX_GRID_CELLS || rows>MAX_GRID_CELLS/cols)return -2;
 float minx=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxz=-FLT_MAX;
 for(uint32_t i=0;i<geom->triangle_count;i++){
   for(int j=0;j<3;j++){
     const cnk_geo_vertex *v=&geom->triangles[i].v[j];
     if(!isfinite(v->x)||!isfinite(v->z))return -3;
     if(v->x<minx)minx=v->x;
     if(v->x>maxx)maxx=v->x;
     if(v->z<minz)minz=v->z;
     if(v->z>maxz)maxz=v->z;
   }
 }
 g.mesh=geom;g.cols=cols;g.rows=rows;g.minx=minx;g.minz=minz;
 g.dx=(maxx-minx+0.01f)/(float)cols;
 g.dz=(maxz-minz+0.01f)/(float)rows;
 /* Finite vertices alone are insufficient: their difference can overflow
  * float, producing INF cell widths and NaN float-to-grid-index casts.
  * Reject malformed/corrupt scene bounds before any cell is addressed. */
 if(!isfinite(g.dx)||!isfinite(g.dz)||g.dx<=0.0f||g.dz<=0.0f||
    !isfinite(g.minx+g.dx*(float)g.cols)||
    !isfinite(g.minz+g.dz*(float)g.rows))return -3;
 uint32_t cells=cols*rows;
 counts=(uint32_t*)calloc(cells,sizeof(*counts));
 if(!counts)return -4;
 uint64_t total=0;
 for(uint32_t i=0;i<geom->triangle_count;i++){
   uint32_t xa,xb,za,zb;
   get_range(&g,&geom->triangles[i],&xa,&xb,&za,&zb);
   for(uint32_t z=za;z<=zb;z++)for(uint32_t x=xa;x<=xb;x++){
     uint32_t idx=z*cols+x;
     if(counts[idx]==UINT32_MAX){free(counts);return -5;}
     counts[idx]++;
     if(++total>MAX_GRID_REFS){free(counts);return -6;}
   }
 }
 g.offsets=(uint32_t*)malloc((cells+1u)*sizeof(uint32_t));
 g.indices=(uint32_t*)malloc((size_t)total*sizeof(uint32_t));
 current=(uint32_t*)malloc(cells*sizeof(uint32_t));
 if(!g.offsets||!g.indices||!current){free(counts);free(current);cnk_collision_grid_free(&g);return -4;}
 g.offsets[0]=0;
 for(uint32_t i=0;i<cells;i++)g.offsets[i+1]=g.offsets[i]+counts[i];
 memcpy(current,g.offsets,cells*sizeof(uint32_t));
 for(uint32_t i=0;i<geom->triangle_count;i++){
   uint32_t xa,xb,za,zb;
   get_range(&g,&geom->triangles[i],&xa,&xb,&za,&zb);
   for(uint32_t z=za;z<=zb;z++)for(uint32_t x=xa;x<=xb;x++){
     uint32_t cell=z*cols+x;
     g.indices[current[cell]++]=i;
   }
 }
 g.count=(uint32_t)total;
 free(counts);free(current);
 *out=g;
 return 0;
}
int cnk_collision_surface(const cnk_collision_grid *g,float x,float z,float reference_y,
                          float *height,uint32_t *triangle_index){
 if(!g||!g->mesh||!g->indices||!g->offsets||!height||
    !isfinite(x)||!isfinite(z)||!isfinite(reference_y))return -1;
 float maxx=g->minx+g->dx*g->cols,maxz=g->minz+g->dz*g->rows;
 if(x<g->minx||x>maxx||z<g->minz||z>maxz)return -2;
 uint32_t cx=grid_cell(x,g->minx,g->dx,g->cols),cz=grid_cell(z,g->minz,g->dz,g->rows);
 uint32_t id=cz*g->cols+cx;
 float best=FLT_MAX,result=0;uint32_t best_index=0;
 for(uint32_t pos=g->offsets[id];pos<g->offsets[id+1];pos++){
   uint32_t i=g->indices[pos];
   const cnk_geo_vertex *a=&g->mesh->triangles[i].v[0],
                        *b=&g->mesh->triangles[i].v[1],
                        *c=&g->mesh->triangles[i].v[2];
   float ax=b->x-a->x,az=b->z-a->z,bx=c->x-a->x,bz=c->z-a->z;
   float d=ax*bz-bx*az;
   if(fabsf(d)<1e-5f)continue;
   float px=x-a->x,pz=z-a->z;
   float t=(px*bz-pz*bx)/d;
   float u=(ax*pz-az*px)/d;
   if(t< -0.00001f || u< -0.00001f || t+u>1.00001f)continue;
   float y=a->y+t*(b->y-a->y)+u*(c->y-a->y);
   float delta=fabsf(y-reference_y);
   if(delta<best){best=delta;result=y;best_index=i;}
 }
 if(best==FLT_MAX)return -2;
 *height=result;
 if(triangle_index)*triangle_index=best_index;
 return 0;
}

/* We visit all XZ grid cells in the segment's bounding rectangle. This is
 * conservative (never misses a triangle intersecting the 3D segment) and
 * avoids requiring a fragile 3D-DDA implementation on the original scenes.
 * Duplicate triangle references in several grid cells are harmless. */
int cnk_collision_segment_cast_steep(const cnk_collision_grid *g,
                               cnk_vec3 from,cnk_vec3 to,float max_abs_normal_y,
                               cnk_segment_hit *out){
 if(!g||!g->mesh||!g->mesh->triangles||!g->offsets||!g->indices||!out||
    !isfinite(from.x)||!isfinite(from.y)||!isfinite(from.z)||
    !isfinite(to.x)||!isfinite(to.y)||!isfinite(to.z)||
    !isfinite(max_abs_normal_y)||max_abs_normal_y<0.0f||
    max_abs_normal_y>1.0f)return -1;
 float dx=to.x-from.x,dy=to.y-from.y,dz=to.z-from.z;
 if(dx*dx+dy*dy+dz*dz<1e-12f)return -1;
 float xmin=fminf(from.x,to.x),xmax=fmaxf(from.x,to.x);
 float zmin=fminf(from.z,to.z),zmax=fmaxf(from.z,to.z);
 float worldx=g->minx+g->dx*g->cols,worldz=g->minz+g->dz*g->rows;
 if(xmax<g->minx||xmin>worldx||zmax<g->minz||zmin>worldz)return -2;
 uint32_t xa=grid_cell(xmin,g->minx,g->dx,g->cols);
 uint32_t xb=grid_cell(xmax,g->minx,g->dx,g->cols);
 uint32_t za=grid_cell(zmin,g->minz,g->dz,g->rows);
 uint32_t zb=grid_cell(zmax,g->minz,g->dz,g->rows);
 float best=2.0f;uint32_t best_idx=UINT32_MAX;cnk_vec3 best_n={0};
 for(uint32_t z=za;z<=zb;z++)for(uint32_t x=xa;x<=xb;x++){
  uint32_t cell=z*g->cols+x;
  for(uint32_t pos=g->offsets[cell];pos<g->offsets[cell+1];pos++){
   uint32_t i=g->indices[pos];
   if(i>=g->mesh->triangle_count)return -1;
   float t=0;cnk_vec3 n={0};
   if(cnk_geometry_triangle_segment(&g->mesh->triangles[i],from,to,&t,&n)!=1 ||
      fabsf(n.y)>max_abs_normal_y+0.000001f)continue;
   if(t<best-1e-6f || (fabsf(t-best)<=1e-6f && i<best_idx)){
    best=t;best_idx=i;best_n=n;
   }
  }
 }
 if(best_idx==UINT32_MAX)return -2;
 const cnk_geo_triangle *tri=&g->mesh->triangles[best_idx];
 *out=(cnk_segment_hit){best,
  {from.x+dx*best,from.y+dy*best,from.z+dz*best},
  best_n,best_idx,tri->material,tri->texture};
 return 0;
}

int cnk_collision_segment_cast(const cnk_collision_grid *g,
                               cnk_vec3 from,cnk_vec3 to,cnk_segment_hit *out){
 return cnk_collision_segment_cast_steep(g,from,to,1.0f,out);
}
