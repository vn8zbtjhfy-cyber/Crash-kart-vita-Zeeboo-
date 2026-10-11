#include "cnk_dev_race_visuals.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

void cnk_dev_race_visuals_free(cnk_dev_race_visuals *v){
 if(!v)return;
 for(unsigned i=0;i<CNK_DEV_VISUAL_FAMILIES;i++)cnk_object_mesh_free(&v->meshes[i]);
 memset(v,0,sizeof(*v));
}
int cnk_dev_race_visuals_load(const fufs_file *rom,cnk_dev_race_visuals *out){
 if(!rom||!out||!rom->entries||!rom->data)return -1;
 memset(out,0,sizeof(*out));
 cnk_dev_race_visuals stage={0};
 for(unsigned i=0;i<CNK_DEV_VISUAL_FAMILIES;i++){
  uint32_t index=0,id=0;
  if(cnk_source_kart_gallery_source(i,1u,&index,&id) ||
     index>=rom->count||rom->entries[index].id!=id ||
     cnk_object_mesh_from_vfs(rom,index,&stage.meshes[i]))goto invalid;
  cnk_object_mesh *m=&stage.meshes[i];
  double center[3]={0},max_extent=0;
  for(unsigned axis=0;axis<3;axis++){
   double lo=m->bbox_min[axis],hi=m->bbox_max[axis];
   if(!isfinite(lo)||!isfinite(hi)||hi<=lo)goto invalid;
   center[axis]=(lo+hi)*0.5;
   if(hi-lo>max_extent)max_extent=hi-lo;
  }
  if(!isfinite(max_extent)||max_extent<1e-6||max_extent>1e12)goto invalid;
  /* This 1.8-unit envelope is only for developer preview. It is NOT a
   * recovered Zeebo vehicle dimension or real gameplay calibration. */
  double scale=1.8/max_extent;
  float *a=stage.local[i];
  a[0]=a[5]=a[10]=(float)scale;a[15]=1.f;
  a[12]=(float)(-center[0]*scale);
  a[13]=(float)(-m->bbox_min[1]*scale);
  a[14]=(float)(-center[2]*scale);
  for(unsigned j=0;j<16;j++)if(!isfinite(a[j]))goto invalid;
  cnk_racer_visual_attachment check={0};
  check.mesh=m;check.local_from_mesh=a;
  if(cnk_racer_visual_attachment_valid(&check))goto invalid;
 }
 stage.ready=1;
 *out=stage;
 return 0;
invalid:
 cnk_dev_race_visuals_free(&stage);
 return -2;
}
int cnk_dev_race_visuals_fill(const cnk_dev_race_visuals *v,unsigned count,
                             cnk_racer_visual_attachment out[CNK_MAX_RACERS]){
 if(!v||!v->ready||!out||count>CNK_MAX_RACERS)return -1;
 cnk_racer_visual_attachment stage[CNK_MAX_RACERS];
 memcpy(stage,out,sizeof(stage));
 for(unsigned i=0;i<count;i++)if(!stage[i].mesh){
  /* Round-robin is a *visual* stress test of all six source families.
   * It must never be treated as a historical pilot/kart association. */
  unsigned family=i%CNK_DEV_VISUAL_FAMILIES;
  stage[i].mesh=&v->meshes[family];
  stage[i].local_from_mesh=v->local[family];
  if(cnk_racer_visual_attachment_valid(&stage[i]))return -2;
 }
 memcpy(out,stage,sizeof(stage));
 return 0;
}
