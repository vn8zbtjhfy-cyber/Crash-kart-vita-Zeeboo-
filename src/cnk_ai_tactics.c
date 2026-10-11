#include "cnk_ai_tactics.h"
#include "cnk_projectile_threat.h"
#include <math.h>
#include <float.h>
static int finite3(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static float clamp1(float n){return n>1.0f?1.0f:n< -1.0f?-1.0f:n;}
int cnk_ai_tactics_apply(const cnk_item_world *w,const cnk_sline *route,
 uint32_t slot,const cnk_kart *kart,cnk_driver_input *control,uint32_t *flags){
 if(!w||!route||!route->points||route->count<3||!kart||!control||!flags||
    slot==0||slot>=CNK_ITEM_MAX_RACERS||w->pickup_count>CNK_ITEM_MAX_PICKUPS||
    !finite3(kart->position)||!isfinite(kart->heading)||!isfinite(kart->speed)||
    !isfinite(control->steer)||!isfinite(control->throttle)||!isfinite(control->brake))return -1;
 /* Derive the kart's local forward / right axes. 2D XZ placement only:
  * this is conservative collision avoidance, not source collision material. */
 float fx=sinf(kart->heading),fz=cosf(kart->heading);
 float rx=fz,rz=-fx;
 float nearest_hazard=FLT_MAX, hazard_side=0;
 for(unsigned j=0;j<CNK_ITEM_MAX_HAZARDS;j++){
  const cnk_item_hazard *h=&w->hazards[j];
  if(!h->active || h->owner==slot)continue;
  if(!finite3(h->position)||!isfinite(h->lifetime)||h->owner>=CNK_ITEM_MAX_RACERS ||
    (h->kind!=CNK_WEAPON_TNT&&h->kind!=CNK_WEAPON_OIL))return -2;
  if(h->lifetime<=0||fabsf(h->position.y-kart->position.y)>2.0f)continue;
  float dx=h->position.x-kart->position.x,dz=h->position.z-kart->position.z;
  float forward=dx*fx+dz*fz,lateral=dx*rx+dz*rz;
  /* A large sideways gap is safe; avoid only an impending direct crossing. */
  if(forward>0.6f&&forward<11.0f&&fabsf(lateral)<1.30f&&forward<nearest_hazard){
   nearest_hazard=forward;hazard_side=lateral;
  }
 }
 cnk_driver_input next=*control;
 uint32_t decision=0;
 cnk_projectile_threat missile_alert={0};
 if(cnk_projectile_threat_read(w,slot,kart,&missile_alert))return -5;
 /* A swept airborne item is more urgent than nearby stationary rewards or
  * hazards. Evasion is steering-only: sudden braking may worsen approach. */
 if(missile_alert.incoming){
  float sign=fabsf(missile_alert.lateral_side)>0.12f?
   (missile_alert.lateral_side>0?-1.0f:1.0f):
   ((slot&1u)?1.0f:-1.0f);
  next.steer=clamp1(next.steer+sign*0.85f);
  *control=next;
  *flags=CNK_AI_TACTIC_AVOID_PROJECTILE;
  return 0;
 }
 if(nearest_hazard<FLT_MAX){
  /* Off-centre obstacle: steer AWAY from it. Centreline: deterministic
   * slot-parity choice. No random numbers, frame-to-frame oscillation or
   * forced reversal of existing route steering. */
  float sign=fabsf(hazard_side)>0.12f?(hazard_side>0?-1.0f:1.0f):
             ((slot&1u)?1.0f:-1.0f);
  float strength=nearest_hazard<4.0f?0.82f:0.58f;
  next.steer=clamp1(next.steer+sign*strength);
  if(nearest_hazard<3.0f && kart->speed>5.0f){
   if(next.throttle>0.15f)next.throttle=0.15f;
   if(next.brake<0.35f)next.brake=0.35f;
  }
  decision=CNK_AI_TACTIC_AVOID_HAZARD;
 }else{
  /* A verified or explicitly placed pickup is pursuable only if usable and
   * close to the source DRV0 corridor. Never invent placements or rewards. */
  float best=FLT_MAX,side=0;
  for(uint32_t j=0;j<w->pickup_count;j++){
   const cnk_item_pickup *p=&w->pickups[j];
   if(!finite3(p->position)||!isfinite(p->radius)||
      !isfinite(p->unavailable_seconds)||p->radius<=0)return -3;
   if(p->unavailable_seconds>0||fabsf(p->position.y-kart->position.y)>2.0f)continue;
   if(p->kind==CNK_PICKUP_WUMPA){if(w->racers[slot].wumpa>=10)continue;}
   else if(p->kind==CNK_PICKUP_CRATE){if(w->racers[slot].held!=CNK_WEAPON_NONE)continue;}
   else return -3;
   float dx=p->position.x-kart->position.x,dz=p->position.z-kart->position.z;
   float ahead=dx*fx+dz*fz,lateral=dx*rx+dz*rz;
   if(ahead<2.0f||ahead>12.0f||fabsf(lateral)>3.0f)continue;
   cnk_nav_projection proj={0};
   if(cnk_nav_nearest(route,p->position,1,&proj))return -4;
   if(proj.distance>4.0f)continue;
   float metric=ahead+fabsf(lateral)*1.5f;
   if(metric<best){best=metric;side=lateral;}
  }
  if(best<FLT_MAX && fabsf(side)>0.22f){
   /* Pursuit correction is capped: primary navigation still dominates. */
   next.steer=clamp1(next.steer+fmaxf(-0.55f,fminf(0.55f,side*0.20f)));
   decision=CNK_AI_TACTIC_SEEK_PICKUP;
  }
 }
 *control=next;
 *flags=decision;
 return 0;
}
