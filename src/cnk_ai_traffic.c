#include "cnk_ai_traffic.h"
#include <math.h>
#include <float.h>

static int valid_kart(const cnk_kart *k){
 return isfinite(k->position.x)&&isfinite(k->position.y)&&
 isfinite(k->position.z)&&isfinite(k->heading)&&isfinite(k->speed);
}
int cnk_ai_traffic_apply(const cnk_sline *route,
 const cnk_kart karts[],const uint8_t movable[],uint32_t count,uint32_t slot,
 cnk_driver_input *control,uint32_t *flags){
 if(!route||!route->points||route->count<3||!karts||!movable||!control||
    !flags||count<2||count>6||slot==0||slot>=count||
    !isfinite(control->steer)||!isfinite(control->throttle)||
    !isfinite(control->brake))return -1;
 for(uint32_t i=0;i<count;i++)if(!valid_kart(&karts[i]))return -2;
 cnk_driver_input next=*control;
 uint32_t next_flag=0;
 if(!movable[slot]||karts[slot].speed<1.0f){*flags=0;return 0;}
 const cnk_kart *self=&karts[slot];
 float sx=sinf(self->heading),sz=cosf(self->heading);
 float rx=sz,rz=-sx;
 float ahead_best=FLT_MAX;
 float traffic_speed=0, traffic_side=0;
 for(uint32_t i=0;i<count;i++)if(i!=slot&&movable[i]){
  const cnk_kart *other=&karts[i];
  if(fabsf(other->position.y-self->position.y)>2.0f)continue;
  float x=other->position.x-self->position.x;
  float z=other->position.z-self->position.z;
  float ahead=x*sx+z*sz;
  float side=x*rx+z*rz;
  if(ahead<=0.45f||ahead>10.0f||fabsf(side)>1.4f)continue;
  /* Same travel direction only. Opposite-lane or perpendicular crossings
   * are not slow traffic and should never stall the route follower. */
  if(cosf(other->heading-self->heading)<0.82f)continue;
  if(other->speed>=self->speed-1.5f)continue;
  if(ahead<ahead_best){ahead_best=ahead;traffic_speed=other->speed;traffic_side=side;}
 }
 if(ahead_best>3.5f && ahead_best<8.0f &&
    self->speed>traffic_speed+2.0f){
  /* Assess BOTH local side corridors against original DRV0. This is only
   * route proximity, NOT recovered track-edge or road material width. */
  cnk_nav_projection own={0};
  if(cnk_nav_nearest(route,self->position,1,&own))return -3;
  int best_sign=0;float best_cost=FLT_MAX;
  for(int sign=-1;sign<=1;sign+=2){
   cnk_vec3 candidate={self->position.x+sx*2.0f+rx*(float)sign*1.65f,
                       self->position.y,
                       self->position.z+sz*2.0f+rz*(float)sign*1.65f};
   cnk_nav_projection projection={0};
   if(cnk_nav_nearest(route,candidate,1,&projection))return -3;
   if(!isfinite(projection.distance)||projection.distance>2.5f||
      projection.distance>own.distance+2.0f)continue;
   int blocked=0;
   for(uint32_t j=0;j<count;j++)if(j!=slot&&movable[j]){
    float dx=karts[j].position.x-candidate.x;
    float dz=karts[j].position.z-candidate.z;
    if(dx*dx+dz*dz<1.85f*1.85f &&
       fabsf(karts[j].position.y-candidate.y)<2.0f){blocked=1;break;}
   }
   if(blocked)continue;
   /* Prefer the side away from the slow kart. On a tie choose the same
    * deterministic slot parity to avoid random per-frame direction flips. */
   float cost=projection.distance+((float)sign*traffic_side>0?0.9f:0.0f);
   if(fabsf(traffic_side)<0.15f && sign!=((slot&1u)?-1:1))cost+=0.05f;
   if(cost<best_cost){best_cost=cost;best_sign=sign;}
  }
  if(best_sign){
   next.steer=fmaxf(-1.0f,fminf(1.0f,next.steer+(float)best_sign*0.42f));
   next_flag=CNK_AI_TRAFFIC_PASS;
  }
 }
 if(ahead_best<FLT_MAX && !next_flag){
  /* Close the speed difference and keep space. No invented negative speed
   * commands or teleportation. Priority evasions are handled by the caller. */
  float gap=fmaxf(1.4f,0.9f+fmaxf(0.0f,self->speed)*0.26f);
  if(ahead_best<gap && ahead_best<3.5f){
   next.throttle=fminf(next.throttle,0.30f);
   if(self->speed>traffic_speed+3.0f)
    next.brake=fmaxf(next.brake,ahead_best<1.7f?0.38f:0.18f);
   next_flag=CNK_AI_TRAFFIC_FOLLOW;
  }
 }
 *control=next;
 *flags=next_flag;
 return 0;
}
