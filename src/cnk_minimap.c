#include "cnk_minimap.h"
#include <math.h>
#include <string.h>
#include <float.h>

int cnk_minimap_build(const cnk_session *s,cnk_minimap_frame *out){
 if(!s||!out||!s->active||!s->route||!s->route->points||
    s->route->count<3||s->racer_count<1||s->racer_count>CNK_MAX_RACERS)
  return -1;
 cnk_minimap_frame m={0};
 float minx=FLT_MAX,maxx=-FLT_MAX,minz=FLT_MAX,maxz=-FLT_MAX;
 for(uint32_t i=0;i<s->route->count;i++){
  cnk_vec3 pt=s->route->points[i];
  if(!isfinite(pt.x)||!isfinite(pt.z))return -2;
  if(pt.x<minx)minx=pt.x;
  if(pt.x>maxx)maxx=pt.x;
  if(pt.z<minz)minz=pt.z;
  if(pt.z>maxz)maxz=pt.z;
 }
 float width=maxx-minx,height=maxz-minz;
 if(!isfinite(width)||!isfinite(height)||width<=0||height<=0)return -3;
 float scale=fmaxf(width,height);
 if(!isfinite(scale)||scale<=0)return -3;
 /* Isotropic scaling: never distort the original source DRV0 shape. */
 float offx=(1.0f-width/scale)*0.5f;
 float offz=(1.0f-height/scale)*0.5f;
 for(uint32_t i=0;i<CNK_MINIMAP_PATH_POINTS;i++){
  cnk_vec3 sample={0};
  if(cnk_nav_sample(s->route,(float)i/(float)CNK_MINIMAP_PATH_POINTS,
                    1,&sample)||!isfinite(sample.x)||!isfinite(sample.z))return -4;
  m.path[i]=(cnk_minimap_point){offx+(sample.x-minx)/scale,
                                offz+(sample.z-minz)/scale};
 }
 m.path_count=CNK_MINIMAP_PATH_POINTS;
 m.minx=minx;m.minz=minz;m.scale=scale;m.offx=offx;m.offz=offz;
 if(cnk_minimap_update_racers(s,&m))return -5;
 *out=m;
 return 0;
}
int cnk_minimap_update_racers(const cnk_session *s,cnk_minimap_frame *m){
 if(!s||!s->active||!m||m->path_count!=CNK_MINIMAP_PATH_POINTS||
    !isfinite(m->scale)||m->scale<=0.0f||
    s->racer_count>CNK_MAX_RACERS)return -1;
 cnk_minimap_point positions[CNK_MAX_RACERS]={0};
 for(uint32_t i=0;i<s->racer_count;i++){
  cnk_vec3 pt=s->racers[i].kart.position;
  if(!isfinite(pt.x)||!isfinite(pt.z))return -5;
  /* Out-of-bounds racers remain at the nearest map edge; their real world
   * position is NOT changed and race rankings are not touched. */
  float x=m->offx+(pt.x-m->minx)/m->scale;
  float z=m->offz+(pt.z-m->minz)/m->scale;
  positions[i]=(cnk_minimap_point){fminf(1.0f,fmaxf(0.0f,x)),
                                  fminf(1.0f,fmaxf(0.0f,z))};
 }
 memcpy(m->racers,positions,sizeof(positions));
 m->racer_count=s->racer_count;
 return 0;
}
