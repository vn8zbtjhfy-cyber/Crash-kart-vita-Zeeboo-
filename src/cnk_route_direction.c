#include "cnk_route_direction.h"
#include <math.h>
#include <stdint.h>

/* All constants are explicit and PROVISIONAL pending behavior measurement. */
#define CNK_DIR_MAX_ROUTE_DISTANCE 8.0f
#define CNK_DIR_MIN_TRAVEL_PER_SECOND 0.45f
#define CNK_DIR_ALIGNMENT 0.65f
#define CNK_DIR_REVERSE_CONFIRM 0.80f
#define CNK_DIR_FORWARD_CLEAR 0.35f
/* Distance to DRV0 in the driving plane (XZ). The collision mesh and DRV0
 * may intentionally have different Y baselines, so 3D nearest would falsely
 * classify valid on-road movement as far from the path. */
static int nearest_xz(const cnk_sline *route,cnk_vec3 p,
                      uint32_t *segment,float *distance){
 float best=INFINITY;uint32_t index=0;int found=0;
 for(uint32_t i=0;i<route->count;i++){
  cnk_vec3 a=route->points[i],b=route->points[(i+1u)%route->count];
  const float dx=b.x-a.x,dz=b.z-a.z;
  const float len2=dx*dx+dz*dz;
  if(!isfinite(len2))return -1;
  if(len2<0.00000001f)continue;
  float t=((p.x-a.x)*dx+(p.z-a.z)*dz)/len2;
  if(t<0)t=0;
  if(t>1)t=1;
  float distance2=(p.x-a.x-t*dx)*(p.x-a.x-t*dx)+
                  (p.z-a.z-t*dz)*(p.z-a.z-t*dz);
  if(!isfinite(distance2))return -1;
  if(distance2<best){best=distance2;index=i;found=1;}
 }
 if(!found)return -1;
 *segment=index;*distance=sqrtf(best);return 0;
}
static int near_segments(uint32_t a,uint32_t b,uint32_t count){
 if(a==b)return 1;
 if((a+1u)%count==b || (b+1u)%count==a)return 1;
 return 0;
}
int cnk_route_direction_step(cnk_route_direction *state,const cnk_sline *route,
                            cnk_vec3 before,cnk_vec3 after,float dt){
 if(!state||!route||!route->points||route->count<3||
    !isfinite(dt)||dt<=0.0f||dt>0.10f||
    !isfinite(before.x)||!isfinite(before.y)||!isfinite(before.z)||
    !isfinite(after.x)||!isfinite(after.y)||!isfinite(after.z))return -1;
 uint32_t segment_a=0,segment_b=0;float distance_a=0,distance_b=0;
 if(nearest_xz(route,before,&segment_a,&distance_a)||
    nearest_xz(route,after,&segment_b,&distance_b))return -2;
 /* Untrusted when kart is far from the original driving path, or the
  * projection jumps to a non-neighboring arm of a hairpin. */
 if(distance_a>CNK_DIR_MAX_ROUTE_DISTANCE||
    distance_b>CNK_DIR_MAX_ROUTE_DISTANCE||
    !near_segments(segment_a,segment_b,route->count)){
  state->reverse_seconds=state->forward_seconds=0.0f;
  state->wrong_way=0;
  return 0;
 }
 float dx=after.x-before.x,dz=after.z-before.z;
 float traveled=hypotf(dx,dz);
 if(!isfinite(traveled))return -2;
 if(traveled<CNK_DIR_MIN_TRAVEL_PER_SECOND*dt)return 0;
 /* Do not interpret discontinuous teleports as genuine reverse driving. */
 if(traveled>30.0f*dt)return 0;
 const cnk_vec3 p=route->points[segment_b];
 const cnk_vec3 q=route->points[(segment_b+1u)%route->count];
 const float tx=q.x-p.x,tz=q.z-p.z;
 const float tangent_length=hypotf(tx,tz);
 if(!isfinite(tangent_length)||tangent_length<0.0001f)return 0;
 const float alignment=(dx*tx+dz*tz)/(traveled*tangent_length);
 if(!isfinite(alignment))return -2;
 if(alignment<=-CNK_DIR_ALIGNMENT){
  state->forward_seconds=0.0f;
  if(state->reverse_seconds<CNK_DIR_REVERSE_CONFIRM)
   state->reverse_seconds=fminf(CNK_DIR_REVERSE_CONFIRM,state->reverse_seconds+dt);
  if(state->reverse_seconds>=CNK_DIR_REVERSE_CONFIRM-0.00001f)state->wrong_way=1;
 }else if(alignment>=CNK_DIR_ALIGNMENT){
  state->reverse_seconds=0.0f;
  if(state->forward_seconds<CNK_DIR_FORWARD_CLEAR)
   state->forward_seconds=fminf(CNK_DIR_FORWARD_CLEAR,state->forward_seconds+dt);
  if(state->forward_seconds>=CNK_DIR_FORWARD_CLEAR-0.00001f)state->wrong_way=0;
 }else{
  /* Sliding sideways cannot itself produce a wrong-way warning. */
  state->reverse_seconds=fmaxf(0.0f,state->reverse_seconds-dt);
  state->forward_seconds=0.0f;
 }
 return 0;
}
