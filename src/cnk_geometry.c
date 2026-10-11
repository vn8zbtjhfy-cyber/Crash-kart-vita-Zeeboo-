#include "cnk_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <limits.h>

static int finite_scale(float s){return isfinite(s) && s>0.0f && s<=1.0f;}
void cnk_geometry_free(cnk_geometry *g){
  if(g){free(g->triangles);memset(g,0,sizeof(*g));}
}
int cnk_geometry_build(const cnk_scene *scene,float scale,cnk_geometry *out){
  int32_t *owners=NULL;
  uint64_t total=0;
  cnk_geometry tmp;
  if(!out) return -1;
  memset(out,0,sizeof(*out));
  memset(&tmp,0,sizeof(tmp));
  if(!scene || !scene->vertices || !scene->chunks || !scene->voffsets ||
     !scene->materials || !finite_scale(scale))return -2;
  if(!scene->vertex_count || !scene->chunk_count || !scene->voffset_count)return -2;
  for(uint32_t c=0;c<scene->chunk_count;c++){
    if(scene->chunks[c].index_count%3 || !scene->chunks[c].indices)return -3;
    total+=scene->chunks[c].index_count/3;
  }
  if(!total || total>1000000u || total>SIZE_MAX/sizeof(cnk_geo_triangle))return -4;
  owners=(int32_t*)malloc((size_t)scene->vertex_count*sizeof(*owners));
  if(!owners)return -5;
  for(uint32_t i=0;i<scene->vertex_count;i++)owners[i]=-1;
  for(uint32_t i=0;i<scene->voffset_count;i++){
    const cnk_voffset *v=&scene->voffsets[i];
    if((uint32_t)v->start+v->count>scene->vertex_count){free(owners);return -6;}
    for(uint32_t j=v->start;j<(uint32_t)v->start+v->count;j++){
      if(owners[j]!=-1){free(owners);return -7;}
      owners[j]=(int32_t)i;
    }
  }
  tmp.triangles=(cnk_geo_triangle*)calloc((size_t)total,sizeof(*tmp.triangles));
  if(!tmp.triangles){free(owners);return -5;}
  tmp.source_triangle_count=(uint32_t)total;
  tmp.scale=scale;
  for(uint32_t k=0;k<scene->chunk_count;k++){
    const cnk_schunk *c=&scene->chunks[k];
    if(c->material>=scene->material_count){cnk_geometry_free(&tmp);free(owners);return -8;}
    uint32_t tex=scene->materials[c->material].id;
    if(tex>=scene->texture_count){cnk_geometry_free(&tmp);free(owners);return -9;}
    for(uint32_t i=0;i<c->index_count;i+=3){
      uint32_t vi[3],ui[3];
      int placed=1,has_uv=1;
      for(uint32_t j=0;j<3;j++){
        vi[j]=(uint32_t)c->vertex_base+c->indices[i+j];
        ui[j]=(uint32_t)c->uv_base+c->indices[i+j];
        if(vi[j]>=scene->vertex_count || owners[vi[j]]<0)placed=0;
        if(ui[j]>=scene->uv_count)has_uv=0;
      }
      if(!placed){tmp.unplaced_triangles++;continue;}
      cnk_geo_triangle *tri=&tmp.triangles[tmp.triangle_count++];
      tri->material=c->material;
      tri->texture=(uint16_t)tex;
      if(!has_uv)tmp.missing_uv_triangles++;
      if(!scene->texture_names[tex][0])tmp.unnamed_texture_triangles++;
      for(uint32_t j=0;j<3;j++){
        cnk_svertex v=scene->vertices[vi[j]];
        cnk_vec3 origin=scene->voffsets[owners[vi[j]]].origin;
        tri->v[j].x=origin.x+scale*v.x;
        tri->v[j].y=origin.y+scale*v.y;
        tri->v[j].z=origin.z+scale*v.z;
        if(has_uv){tri->v[j].u=scene->uvs[ui[j]].u;
                   tri->v[j].v=scene->uvs[ui[j]].v;
                   tri->v[j].uv_valid=1;}
      }
    }
  }
  free(owners);
  *out=tmp;
  return 0;
}
int cnk_geometry_nearest_surface(const cnk_geometry *g,
                                 float x,float z,float reference_y,
                                 float *surface_y,uint32_t *triangle_index){
  if(!g || !g->triangles || !surface_y || !isfinite(x) || !isfinite(z) ||
     !isfinite(reference_y))return -1;
  float best=FLT_MAX,world_y=0.0f;
  uint32_t best_i=0;
  for(uint32_t i=0;i<g->triangle_count;i++){
    const cnk_geo_triangle *tri=&g->triangles[i];
    const cnk_geo_vertex *a=&tri->v[0],*b=&tri->v[1],*c=&tri->v[2];
    float a_x=b->x-a->x,a_z=b->z-a->z,b_x=c->x-a->x,b_z=c->z-a->z;
    float determinant=a_x*b_z-b_x*a_z;
    if(fabsf(determinant)<1e-5f)continue;
    float px=x-a->x,pz=z-a->z;
    float t=(px*b_z-pz*b_x)/determinant;
    float u=(a_x*pz-a_z*px)/determinant;
    if(t < -0.00001f || u < -0.00001f || t+u>1.00001f)continue;
    float y=a->y+t*(b->y-a->y)+u*(c->y-a->y);
    float delta=fabsf(y-reference_y);
    if(delta<best){best=delta;world_y=y;best_i=i;}
  }
  if(best==FLT_MAX)return -2;
  *surface_y=world_y;
  if(triangle_index)*triangle_index=best_i;
  return 0;
}

/* Double-sided Moller-Trumbore intersection. Parameters refer to a bounded
 * segment, not an infinite ray; near-parallel/coplanar triangles are ignored.
 * Degenerate segments are invalid at the public query layer. */
static int vec_finite(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static cnk_vec3 sub3(cnk_vec3 a,cnk_vec3 b){return (cnk_vec3){a.x-b.x,a.y-b.y,a.z-b.z};}
static cnk_vec3 cross3(cnk_vec3 a,cnk_vec3 b){
 return (cnk_vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static float dot3(cnk_vec3 a,cnk_vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
int cnk_geometry_triangle_segment(const cnk_geo_triangle *tri,cnk_vec3 from,
                                   cnk_vec3 to,float *fraction,cnk_vec3 *normal){
 if(!tri||!fraction||!normal||!vec_finite(from)||!vec_finite(to))return -1;
 cnk_vec3 v[3];
 for(int i=0;i<3;i++){
  v[i]=(cnk_vec3){tri->v[i].x,tri->v[i].y,tri->v[i].z};
  if(!vec_finite(v[i]))return -1;
 }
 cnk_vec3 edge1=sub3(v[1],v[0]),edge2=sub3(v[2],v[0]);
 cnk_vec3 d=sub3(to,from),p=cross3(d,edge2);
 float determinant=dot3(edge1,p);
 if(!isfinite(determinant)||fabsf(determinant)<1e-7f)return 0;
 float inverse=1.0f/determinant;
 cnk_vec3 tvec=sub3(from,v[0]);
 float u=dot3(tvec,p)*inverse;
 if(u < -1e-6f || u > 1.000001f)return 0;
 cnk_vec3 q=cross3(tvec,edge1);
 float vv=dot3(d,q)*inverse;
 if(vv < -1e-6f || u+vv > 1.000001f)return 0;
 float t=dot3(edge2,q)*inverse;
 if(!isfinite(t)||t < -1e-6f || t>1.000001f)return 0;
 cnk_vec3 n=cross3(edge1,edge2);
 float norm=sqrtf(dot3(n,n));
 if(!isfinite(norm)||norm<1e-8f)return 0;
 *fraction=fmaxf(0.0f,fminf(1.0f,t));
 *normal=(cnk_vec3){n.x/norm,n.y/norm,n.z/norm};
 return 1;
}
int cnk_geometry_segment_cast(const cnk_geometry *mesh,cnk_vec3 from,
                               cnk_vec3 to,cnk_segment_hit *out){
 if(!mesh||!mesh->triangles||!mesh->triangle_count||!out||
    !vec_finite(from)||!vec_finite(to))return -1;
 cnk_vec3 delta=sub3(to,from);
 if(dot3(delta,delta)<1e-12f)return -1;
 float best=2.0f;uint32_t best_idx=UINT32_MAX;cnk_vec3 best_n={0};
 for(uint32_t i=0;i<mesh->triangle_count;i++){
  float t=0;cnk_vec3 n={0};
  if(cnk_geometry_triangle_segment(&mesh->triangles[i],from,to,&t,&n)!=1)continue;
  if(t<best-1e-6f || (fabsf(t-best)<=1e-6f && i<best_idx)){
   best=t;best_idx=i;best_n=n;
  }
 }
 if(best_idx==UINT32_MAX)return -2;
 const cnk_geo_triangle *tri=&mesh->triangles[best_idx];
 *out=(cnk_segment_hit){best,
  {from.x+delta.x*best,from.y+delta.y*best,from.z+delta.z*best},
  best_n,best_idx,tri->material,tri->texture};
 return 0;
}
