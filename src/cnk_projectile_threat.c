#include "cnk_projectile_threat.h"
#include <math.h>
#include <float.h>
#include <limits.h>

static int finite3(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
/* 0 no threat, 1 collision candidate, negative malformed input. */
static int check(cnk_vec3 pos,float heading,float lifetime,uint32_t owner,
                 uint32_t racer,cnk_weapon kind,const cnk_kart *kart,
                 uint32_t slot,cnk_projectile_threat *best){
 if(!finite3(pos)||!isfinite(heading)||!isfinite(lifetime)||
    owner>=CNK_ITEM_MAX_RACERS)return -2;
 if(lifetime<=0.0f||owner==racer||fabsf(pos.y-kart->position.y)>2.0f)return 0;
 float vx=sinf(heading)*(kind==CNK_WEAPON_BOMB?11.0f:15.0f);
 float vz=cosf(heading)*(kind==CNK_WEAPON_BOMB?11.0f:15.0f);
 float kx=sinf(kart->heading)*kart->speed;
 float kz=cosf(kart->heading)*kart->speed;
 float rx=pos.x-kart->position.x,rz=pos.z-kart->position.z;
 float dvx=vx-kx,dvz=vz-kz;
 float speed_sq=dvx*dvx+dvz*dvz;
 if(!isfinite(speed_sq)||speed_sq<0.0001f)return 0;
 float eta=-(rx*dvx+rz*dvz)/speed_sq;
 if(!isfinite(eta)||eta<0.0f||eta>0.90f||eta>lifetime)return 0;
 float dx=rx+dvx*eta,dz=rz+dvz*eta;
 float separation_sq=dx*dx+dz*dz;
 if(!isfinite(separation_sq)||separation_sq>1.65f*1.65f)return 0;
 if(!best->incoming||eta<best->eta_seconds){
  best->incoming=1;best->kind=kind;best->projectile_slot=slot;
  best->eta_seconds=eta;
  best->lateral_side=rx*cosf(kart->heading)-rz*sinf(kart->heading);
  best->closest_distance=sqrtf(separation_sq);
 }
 return 1;
}
int cnk_projectile_threat_read(const cnk_item_world *w,uint32_t racer,
                               const cnk_kart *kart,cnk_projectile_threat *out){
 if(!w||!kart||!out||racer>=CNK_ITEM_MAX_RACERS||
    !finite3(kart->position)||!isfinite(kart->heading)||
    !isfinite(kart->speed))return -1;
 cnk_projectile_threat result={0};
 for(uint32_t i=0;i<CNK_ITEM_MAX_BOMBS;i++)if(w->bombs[i].active){
  const cnk_item_bomb *b=&w->bombs[i];
  if(check(b->position,b->heading,b->lifetime,b->owner,racer,
           CNK_WEAPON_BOMB,kart,i,&result)<0)return -2;
 }
 for(uint32_t i=0;i<CNK_ITEM_MAX_MISSILES;i++)if(w->missiles[i].active){
  const cnk_item_missile *m=&w->missiles[i];
  if(m->target!=UINT32_MAX&&m->target>=CNK_ITEM_MAX_RACERS)return -2;
  if(check(m->position,m->heading,m->lifetime,m->owner,racer,
           CNK_WEAPON_MISSILE,kart,i,&result)<0)return -2;
 }
 *out=result;
 return 0;
}
