#include "cnk_item_visibility.h"
#include <math.h>
#include <string.h>

static int valid_frame(const cnk_item_visual_frame *frame){
 if(!frame||frame->count>CNK_ITEM_VISUAL_CAPACITY)return 0;
 for(size_t i=0;i<frame->count;i++){
  const cnk_item_visual *v=&frame->actors[i];
  if(v->sprite<0||v->sprite>=CNK_ITEM_SPRITE_COUNT||
     !isfinite(v->position.x)||!isfinite(v->position.y)||
     !isfinite(v->position.z)||!isfinite(v->half_size)||
     v->half_size<=0.f||v->half_size>100.f)return 0;
 }
 return 1;
}
static int is_visible(const cnk_camera *cam,const cnk_item_visual *v){
 /* Conservative bounding sphere for a rectangle of half-size s located
  * above the item's world position by s. Radius s*sqrt(2) covers corners.
  * Homogeneous clip test: w +/- x/y/z >= 0, with radius projection. */
 double x=v->position.x,y=(double)v->position.y+v->half_size,z=v->position.z;
 double radius=(double)v->half_size*1.4142135623730951;
 const float *m=cam->view_projection;
 for(int axis=0;axis<3;axis++)for(int sign=-1;sign<=1;sign+=2){
  double a=(double)m[3]+sign*(double)m[axis];
  double b=(double)m[7]+sign*(double)m[4+axis];
  double c=(double)m[11]+sign*(double)m[8+axis];
  double d=(double)m[15]+sign*(double)m[12+axis];
  double distance=a*x+b*y+c*z+d;
  double largest=distance+radius*sqrt(a*a+b*b+c*c);
  double tolerance=1e-5*(fabs(distance)+fabs(d)+1.0);
  if(largest < -tolerance)return 0;
 }
 return 1;
}
static double distance_sq(const cnk_camera *cam,const cnk_item_visual *v){
 double x=(double)v->position.x-cam->eye.x;
 double y=(double)v->position.y+v->half_size-cam->eye.y;
 double z=(double)v->position.z-cam->eye.z;
 return x*x+y*y+z*z;
}
int cnk_item_visibility_prepare(const cnk_item_visual_frame *input,
     const cnk_camera *camera,cnk_item_visual_frame *output,unsigned *culled){
 if(!input||!camera||!output||!valid_frame(input))return -1;
 for(int i=0;i<16;i++)if(!isfinite(camera->view_projection[i])||
       !isfinite(camera->view[i])||!isfinite(camera->projection[i]))return -2;
 if(!isfinite(camera->eye.x)||!isfinite(camera->eye.y)||
    !isfinite(camera->eye.z))return -2;
 cnk_item_visual_frame prepared={0};
 double depth[CNK_ITEM_VISUAL_CAPACITY];
 unsigned hidden=0;
 for(size_t i=0;i<input->count;i++){
  const cnk_item_visual *v=&input->actors[i];
  if(!is_visible(camera,v)){hidden++;continue;}
  double d=distance_sq(camera,v);
  if(!isfinite(d))return -3;
  /* Insertion sort: cap 76 actors, stable on equal distances. */
  size_t j=prepared.count++;
  while(j>0&&depth[j-1]<d){
   prepared.actors[j]=prepared.actors[j-1];
   depth[j]=depth[j-1];j--;
  }
  prepared.actors[j]=*v;depth[j]=d;
 }
 /* No mutation on errors, including if input == output. */
 *output=prepared;
 if(culled)*culled=hidden;
 return 0;
}
