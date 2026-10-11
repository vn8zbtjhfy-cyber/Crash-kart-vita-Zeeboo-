#include "cnk_boost_pads.h"
#include <math.h>
#include <string.h>

static int point_ok(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static int in_pad(cnk_vec3 p,cnk_vec3 q,float radius){
 double dx=(double)p.x-q.x,dz=(double)p.z-q.z;
 return fabs((double)p.y-q.y)<=2.0 && dx*dx+dz*dz<=(double)radius*radius;
}
static int crossed(cnk_vec3 from,cnk_vec3 to,cnk_vec3 p,float r){
 double dx=(double)to.x-from.x,dz=(double)to.z-from.z;
 double length=dx*dx+dz*dz;
 double t=0.0;
 if(length>1.0e-14){
  t=(((double)p.x-from.x)*dx+((double)p.z-from.z)*dz)/length;
  if(t<0)t=0;
  if(t>1)t=1;
 }
 cnk_vec3 nearest={(float)(from.x+dx*t),(float)(from.y+((double)to.y-from.y)*t),
                   (float)(from.z+dz*t)};
 return in_pad(nearest,p,r);
}
int cnk_boost_pad_place(cnk_boost_pad_world *w,cnk_vec3 p,float r){
 if(!w||w->count>=CNK_PAD_MAX||!point_ok(p)||!isfinite(r)||r<0.10f||r>15.0f)return -1;
 /* Reject overlapping entries to prevent ambiguous multi-pad impulses. */
 for(uint32_t i=0;i<w->count;i++){
  const cnk_boost_pad *other=&w->pads[i];
  double dx=(double)p.x-other->position.x,dz=(double)p.z-other->position.z;
  double separation=(double)r+other->radius;
  if(fabs((double)p.y-other->position.y)<=2.0 && dx*dx+dz*dz<separation*separation)
   return -2;
 }
 cnk_boost_pad pad={0};pad.position=p;pad.radius=r;
 w->pads[w->count++]=pad;
 return 0;
}
int cnk_boost_pad_step(cnk_boost_pad_world *w,cnk_item_world *items,
       const cnk_vec3 before[CNK_PAD_RACERS],
       const cnk_kart karts[CNK_PAD_RACERS],
       const uint8_t moving[CNK_PAD_RACERS],uint32_t count,float dt,
       uint32_t events[CNK_PAD_RACERS]){
 if(!w||!items||!before||!karts||!moving||!events||count<1||count>CNK_PAD_RACERS||
    w->count>CNK_PAD_MAX||!isfinite(dt)||dt<=0||dt>0.1f)return -1;
 for(uint32_t i=0;i<count;i++)if(!point_ok(before[i])||!point_ok(karts[i].position)||
     !isfinite(items->racers[i].turbo_seconds)||items->racers[i].turbo_seconds<0)return -2;
 for(uint32_t p=0;p<w->count;p++){
  const cnk_boost_pad *pad=&w->pads[p];
  if(!point_ok(pad->position)||!isfinite(pad->radius)||pad->radius<0.1f||pad->radius>15)return -3;
  for(uint32_t i=0;i<count;i++)if(!isfinite(pad->cooldown[i])||pad->cooldown[i]<0)return -3;
 }
 /* No heap allocation during frames: bounded transactional staging. */
 cnk_boost_pad_world next=*w;
 float boosts[CNK_PAD_RACERS]={0};
 uint32_t edge_events[CNK_PAD_RACERS]={0};
 for(uint32_t p=0;p<next.count;p++){
  cnk_boost_pad *pad=&next.pads[p];
  for(uint32_t i=0;i<count;i++){
   int end_inside=in_pad(karts[i].position,pad->position,pad->radius);
   int was_inside=pad->inside[i];
   if(pad->cooldown[i]>0){pad->cooldown[i]=fmaxf(0,pad->cooldown[i]-dt);}
   if(moving[i] && !was_inside && pad->cooldown[i]==0 &&
      crossed(before[i],karts[i].position,pad->position,pad->radius)){
    if(boosts[i]<1.30f)boosts[i]=1.30f;
    edge_events[i]|=CNK_PAD_EVENT_APPLIED;
    pad->cooldown[i]=0.8f;
   }
   pad->inside[i]=(uint8_t)end_inside;
  }
 }
 *w=next;
 for(uint32_t i=0;i<count;i++){
  if(items->racers[i].turbo_seconds<boosts[i])items->racers[i].turbo_seconds=boosts[i];
  events[i]|=edge_events[i];
 }
 return 0;
}
