#include "cnk_source_kart_gallery.h"
#include "cnk_racer_visual.h"
#include "cnk_vitagl_objects.h"
#include <vitaGL.h>
#include <math.h>
#include <string.h>
#include <limits.h>


void cnk_source_kart_gallery_free(cnk_source_kart_gallery *g){
 if(!g)return;
 for(unsigned i=0;i<CNK_SOURCE_KART_FAMILIES;i++)
  for(unsigned k=0;k<CNK_SOURCE_KART_LODS;k++)
   cnk_object_mesh_free(&g->mesh[i][k]);
 memset(g,0,sizeof(*g));
}
int cnk_source_kart_gallery_load(const fufs_file *vfs,cnk_source_kart_gallery *out){
 if(!vfs||!out||!vfs->data||!vfs->entries)return -1;
 memset(out,0,sizeof(*out));
 cnk_source_kart_gallery stage={0};
 for(unsigned i=0;i<CNK_SOURCE_KART_FAMILIES;i++)
  for(unsigned k=0;k<CNK_SOURCE_KART_LODS;k++){
   uint32_t idx=0,id=0;
   if(cnk_source_kart_gallery_source(i,k,&idx,&id)||
      idx>=vfs->count||vfs->entries[idx].id!=id){
    cnk_source_kart_gallery_free(&stage);return -2;
   }
   if(cnk_object_mesh_from_vfs(vfs,idx,&stage.mesh[i][k])){
    cnk_source_kart_gallery_free(&stage);return -3;
   }
  }
 stage.ready=1;
 *out=stage;
 return 0;
}
int cnk_source_kart_gallery_camera_at(const cnk_object_mesh *mesh,float angle,cnk_camera *out){
 if(!isfinite(angle)||fabsf(angle)>10000.f)return -6;
 if(!mesh||!out||!mesh->vertices||!mesh->vertex_count)return -1;
 double center[3],maxside=0;
 for(unsigned a=0;a<3;a++){
  float lo=mesh->bbox_min[a],hi=mesh->bbox_max[a];
  if(!isfinite(lo)||!isfinite(hi)||hi<lo)return -2;
  center[a]=((double)hi+(double)lo)*0.5;
  double length=(double)hi-lo;
  if(length>maxside)maxside=length;
 }
 if(maxside<0.00001||maxside>1e9||!isfinite(maxside))return -3;
 double distance=maxside*3.3;
 cnk_vec3 target={(float)center[0],(float)center[1],(float)center[2]};
 double original_x=maxside*0.55, original_z=distance;
 double x=original_x*cos((double)angle)+original_z*sin((double)angle);
 double z=original_z*cos((double)angle)-original_x*sin((double)angle);
 cnk_vec3 eye={(float)(center[0]+x),
               (float)(center[1]+maxside*0.30),
               (float)(center[2]+z)};
 if(!isfinite(eye.x)||!isfinite(eye.y)||!isfinite(eye.z)||
    !isfinite(target.x)||!isfinite(target.y)||!isfinite(target.z))return -4;
 cnk_camera stage={0};
 if(cnk_camera_build(&stage,eye,target,0.93f,320.f/174.f,
                    (float)(maxside*0.01),(float)(distance*5.0)))return -5;
 *out=stage;return 0;
}
int cnk_source_kart_gallery_camera(const cnk_object_mesh *mesh,cnk_camera *out){
 return cnk_source_kart_gallery_camera_at(mesh,0.f,out);
}
int cnk_source_kart_gallery_draw_mapped_at(const cnk_source_kart_gallery *g,
                                unsigned detail,float angle,
                                const cnk_gallery_material_binding binds[CNK_SOURCE_KART_FAMILIES],
                                unsigned *calls){
 if(calls)*calls=0;
 if(!g||!calls||!g->ready||detail>=CNK_SOURCE_KART_LODS||
    !isfinite(angle)||fabsf(angle)>10000.f)return -1;
 static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 cnk_camera cams[CNK_SOURCE_KART_FAMILIES];
 /* Reject inconsistent bindings before the first GPU instruction. */
 for(unsigned i=0;i<CNK_SOURCE_KART_FAMILIES;i++){
  const cnk_object_mesh *mesh=&g->mesh[i][detail];
  cnk_racer_visual_attachment a={0};a.mesh=mesh;a.local_from_mesh=identity;
  if(cnk_racer_visual_attachment_valid(&a)||
     cnk_source_kart_gallery_camera_at(mesh,angle,&cams[i])||
     (binds && !binds[i].resolve && binds[i].context))return -2;
 }
 unsigned total=0;
 for(unsigned i=0;i<CNK_SOURCE_KART_FAMILIES;i++){
  unsigned count=0;
  glViewport((int)(i%3u)*320,i<3?277:105,320,174);
  cnk_object_material_texture resolve=binds?binds[i].resolve:NULL;
  void *ctx=binds?binds[i].context:NULL;
  if(cnk_vitagl_draw_object(&g->mesh[i][detail],&cams[i],identity,
                           resolve,ctx,&count) || count>UINT_MAX-total){
   glViewport(0,0,960,544);return -3;
  }
  total+=count;
 }
 glViewport(0,0,960,544);
 *calls=total;return 0;
}
int cnk_source_kart_gallery_draw_mapped(const cnk_source_kart_gallery *g,
                                unsigned detail,
                                const cnk_gallery_material_binding binds[CNK_SOURCE_KART_FAMILIES],
                                unsigned *calls){
 return cnk_source_kart_gallery_draw_mapped_at(g,detail,0.f,binds,calls);
}
int cnk_source_kart_gallery_draw(const cnk_source_kart_gallery *g,
                                 unsigned detail,unsigned *calls){
 return cnk_source_kart_gallery_draw_mapped(g,detail,NULL,calls);
}
