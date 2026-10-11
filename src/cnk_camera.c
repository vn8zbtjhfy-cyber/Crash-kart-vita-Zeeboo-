#include "cnk_camera.h"
#include <math.h>
#include <string.h>
static float dot(cnk_vec3 a,cnk_vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static cnk_vec3 minus(cnk_vec3 a,cnk_vec3 b){return (cnk_vec3){a.x-b.x,a.y-b.y,a.z-b.z};}
static cnk_vec3 cross(cnk_vec3 a,cnk_vec3 b){return (cnk_vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static int norm(cnk_vec3 *a){float r=sqrtf(dot(*a,*a));if(!isfinite(r)||r<1e-6f)return -1;a->x/=r;a->y/=r;a->z/=r;return 0;}
static void mul(float dst[16],const float a[16],const float b[16]){
 float t[16];
 for(int col=0;col<4;col++)for(int row=0;row<4;row++){
  float sum=0;for(int k=0;k<4;k++)sum+=a[k*4+row]*b[col*4+k];
  t[col*4+row]=sum;
 }
 memcpy(dst,t,sizeof(t));
}
int cnk_camera_build(cnk_camera *out,cnk_vec3 eye,cnk_vec3 target,
                     float fov,float aspect,float near_z,float far_z){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!isfinite(eye.x)||!isfinite(eye.y)||!isfinite(eye.z)||
    !isfinite(target.x)||!isfinite(target.y)||!isfinite(target.z)||
    !isfinite(fov)||!isfinite(aspect)||!isfinite(near_z)||!isfinite(far_z)||
    fov<0.1f||fov>3.0f||aspect<=0||aspect>10||near_z<=0||far_z<=near_z)return -2;
 cnk_vec3 forward=minus(target,eye);
 if(norm(&forward))return -3;
 cnk_vec3 back={-forward.x,-forward.y,-forward.z};
 cnk_vec3 world_up={0,1,0};
 /* A right-handed view matrix needs right = up x back.
  * The former back x up mirrored the scene horizontally. */
 cnk_vec3 right=cross(world_up,back);
 if(norm(&right))return -4;
 cnk_vec3 up=cross(back,right);
 float *v=out->view;
 v[0]=right.x;v[4]=right.y;v[8]=right.z;v[12]=-dot(right,eye);
 v[1]=up.x;   v[5]=up.y;   v[9]=up.z;   v[13]=-dot(up,eye);
 v[2]=back.x; v[6]=back.y; v[10]=back.z;v[14]=-dot(back,eye);
 v[15]=1.0f;
 float f=1.0f/tanf(fov*0.5f);
 float *p=out->projection;
 p[0]=f/aspect;p[5]=f;
 p[10]=(far_z+near_z)/(near_z-far_z);p[11]=-1;
 p[14]=2*far_z*near_z/(near_z-far_z);
 mul(out->view_projection,p,v);
 out->eye=eye;out->target=target;
 return 0;
}
int cnk_camera_follow(cnk_camera *out,const cnk_kart *kart,float distance,float lift,float ahead,float fov,float aspect){
 if(!kart||!isfinite(kart->heading)||!isfinite(distance)||!isfinite(lift)||
    !isfinite(ahead)||distance<=0||lift<=0||ahead<0)return -1;
 float dx=sinf(kart->heading),dz=cosf(kart->heading);
 cnk_vec3 eye={kart->position.x-dx*distance,kart->position.y+lift,kart->position.z-dz*distance};
 cnk_vec3 target={kart->position.x+dx*ahead,kart->position.y+lift*0.25f,kart->position.z+dz*ahead};
 return cnk_camera_build(out,eye,target,fov,aspect,0.15f,8000.0f);
}
int cnk_camera_follow_obstructed(cnk_camera *out,const cnk_kart *kart,
                      const cnk_collision_grid *world,
                      float distance,float lift,float lookahead,
                      float fov,float aspect,float margin){
 if(!out||!kart||!isfinite(margin)||margin<0.0f||margin>20.0f)return -1;
 cnk_camera temp={0};
 int rc=cnk_camera_follow(&temp,kart,distance,lift,lookahead,fov,aspect);
 if(rc)return rc;
 if(world){
  /* Origin is above the wheels, avoiding immediate coplanar hits with the
   * road underneath the kart. Source game camera height is still unknown. */
  cnk_vec3 anchor={kart->position.x,
                   kart->position.y+fminf(lift*0.25f,1.5f),
                   kart->position.z};
  cnk_vec3 delta={temp.eye.x-anchor.x,temp.eye.y-anchor.y,temp.eye.z-anchor.z};
  float length=sqrtf(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
  if(!isfinite(length)||length<0.0001f)return -2;
  cnk_segment_hit hit={0};
  rc=cnk_collision_segment_cast(world,anchor,temp.eye,&hit);
  if(rc!=0&&rc!=-2)return -3;
  if(rc==0){
   /* Stay on the kart side of the first obstruction. If it lies directly
    * at the anchor, keep a tiny clearance from the player rather than
    * place the eye behind the surface. */
   float fraction=hit.fraction-margin/length;
   if(fraction<0.02f)fraction=0.02f;
   if(fraction>1.0f)fraction=1.0f;
   cnk_vec3 eye={anchor.x+delta.x*fraction,
                 anchor.y+delta.y*fraction,
                 anchor.z+delta.z*fraction};
   rc=cnk_camera_build(&temp,eye,temp.target,fov,aspect,0.15f,8000.0f);
   if(rc)return -4;
  }
 }
 *out=temp;
 return 0;
}
int cnk_camera_project(const cnk_camera *cam,cnk_vec3 point,float *x,float *y){
 if(!cam||!x||!y)return -1;
 float p[4]={point.x,point.y,point.z,1.0f},q[4]={0};
 for(int r=0;r<4;r++)for(int c=0;c<4;c++)q[r]+=cam->view_projection[c*4+r]*p[c];
 if(!isfinite(q[3])||q[3]<=0.00001f)return -2;
 for(int r=0;r<3;r++)if(!isfinite(q[r])||fabsf(q[r])>q[3])return -2;
 *x=q[0]/q[3];*y=q[1]/q[3];return 0;
}
