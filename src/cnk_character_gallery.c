#include "cnk_character_gallery.h"
#include "cnk_vitagl_objects.h"
#include "cnk_racer_visual.h"
#include <vitaGL.h>
#include <math.h>
#include <string.h>
#include <limits.h>

int cnk_character_gallery_camera_at(const cnk_object_mesh *mesh,float angle,cnk_camera *out){
 if(!isfinite(angle)||fabsf(angle)>10000.f)return -6;
 if(!mesh||!out||!mesh->vertices||!mesh->vertex_count)return -1;
 double center[3],max_side=0;
 for(int k=0;k<3;k++){
  float lo=mesh->bbox_min[k],hi=mesh->bbox_max[k];
  if(!isfinite(lo)||!isfinite(hi)||hi<lo)return -2;
  center[k]=((double)lo+(double)hi)*0.5;
  double size=(double)hi-(double)lo;
  if(size>max_side)max_side=size;
 }
 if(!isfinite(max_side)||max_side<1e-5 || max_side>1e10)return -3;
 /* Each preview has its own camera, not a synthetic source-space pose.
  * A generous separation also accommodates wide and tall source meshes. */
 double distance=max_side*3.0;
 cnk_vec3 target={(float)center[0],(float)center[1],(float)center[2]};
 cnk_vec3 eye={(float)(center[0]+distance*sin((double)angle)),
               (float)(center[1]+max_side*0.20),
               (float)(center[2]+distance*cos((double)angle))};
 if(!isfinite(eye.x)||!isfinite(eye.y)||!isfinite(eye.z)||
    !isfinite(target.x)||!isfinite(target.y)||!isfinite(target.z))return -4;
 cnk_camera stage={0};
 if(cnk_camera_build(&stage,eye,target,0.88f,320.f/380.f,
                     (float)(max_side*0.01), (float)(distance*5.0)))return -5;
 *out=stage;return 0;
}

int cnk_character_gallery_camera(const cnk_object_mesh *mesh,cnk_camera *out){
 return cnk_character_gallery_camera_at(mesh,0.f,out);
}
int cnk_character_gallery_draw_at(const cnk_character_pack *pack,float angle,unsigned *calls){
 if(calls)*calls=0;
 if(!pack||!calls||!pack->ready||!isfinite(angle)||fabsf(angle)>10000.f)return -1;
 static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 cnk_camera cam[CNK_CHARACTER_COUNT];
 /* Preflight all three original resource assets before any GPU call. */
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++){
  const cnk_object_skin *skin=&pack->characters[i].skin;
  if(!skin->ready||!skin->count)return -2;
  cnk_racer_visual_attachment v={0};
  v.mesh=&skin->mesh;v.local_from_mesh=identity;
  if(cnk_racer_visual_attachment_valid(&v)||
     cnk_character_gallery_camera_at(&skin->mesh,angle,&cam[i]))return -3;
  if(!cnk_character_original_resolve((void*)&pack->characters[i],
                            skin->selectors[0]))return -4;
 }
 unsigned total=0;
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++){
  unsigned n=0;
  glViewport((int)i*320,85,320,380);
  int rc=cnk_vitagl_draw_object(&pack->characters[i].skin.mesh,&cam[i],
         identity,cnk_character_original_resolve,(void*)&pack->characters[i],&n);
  if(rc){glViewport(0,0,960,544);return -5;}
  if(total>UINT_MAX-n){glViewport(0,0,960,544);return -6;}
  total+=n;
 }
 glViewport(0,0,960,544);
 *calls=total;
 return 0;
}

int cnk_character_gallery_draw(const cnk_character_pack *pack,unsigned *calls){
 return cnk_character_gallery_draw_at(pack,0.f,calls);
}
