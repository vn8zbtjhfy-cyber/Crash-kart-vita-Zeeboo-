#include "cnk_items.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

static int point_ok(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
/* First contact with an XZ disc during one fixed physics step.
 * Returns the time within [0,1] instead of merely testing a swept path.
 * Racer slot is only a deterministic tie-breaker, never an advantage.
 * This is portable collision math, NOT original Zeebo collision tuning. */
static int first_contact_xz(cnk_vec3 a,cnk_vec3 b,cnk_vec3 center,
                            float radius,float *fraction){
 float x=a.x-center.x,z=a.z-center.z;
 float dx=b.x-a.x,dz=b.z-a.z;
 float c=x*x+z*z-radius*radius;
 if(c<=0.0f){*fraction=0.0f;return 1;}
 float aa=dx*dx+dz*dz;
 if(!(aa>1.0e-12f))return 0;
 float bb=2.0f*(x*dx+z*dz);
 if(bb>=0.0f)return 0;
 float discriminant=bb*bb-4.0f*aa*c;
 if(discriminant<0.0f)return 0;
 float t=(-bb-sqrtf(discriminant))/(2.0f*aa);
 if(!isfinite(t)||t<0.0f||t>1.0f)return 0;
 *fraction=t;
 return 1;
}
/* Candidate contacts are ordered by actual arrival within a physics frame,
 * NOT by the order in which placements happen to be stored in the ROM map.
 * A deterministic pickup/slot ordering is used only for identical times. */
typedef struct {
 float fraction;
 uint32_t pickup, racer;
} cnk_pickup_touch;
static int pickup_touch_compare(const void *lhs,const void *rhs){
 const cnk_pickup_touch *a=(const cnk_pickup_touch *)lhs;
 const cnk_pickup_touch *b=(const cnk_pickup_touch *)rhs;
 if(a->fraction<b->fraction)return -1;
 if(a->fraction>b->fraction)return 1;
 if(a->pickup<b->pickup)return -1;
 if(a->pickup>b->pickup)return 1;
 return (a->racer>b->racer)-(a->racer<b->racer);
}
static int reward_ok(cnk_weapon weapon){
 return weapon==CNK_WEAPON_TURBO||weapon==CNK_WEAPON_BOMB||
        weapon==CNK_WEAPON_MISSILE||weapon==CNK_WEAPON_TNT||
        weapon==CNK_WEAPON_OIL||weapon==CNK_WEAPON_SHIELD||
        weapon==CNK_WEAPON_TROPY_CLOCK||weapon==CNK_WEAPON_CURSED_CANDLE||
        weapon==CNK_WEAPON_AKU_AKU||weapon==CNK_WEAPON_KART_JACKING;
}
static float wrapped_angle(float a){
 while(a>3.14159265358979323846f)a-=6.28318530717958647692f;
 while(a< -3.14159265358979323846f)a+=6.28318530717958647692f;
 return a;
}
static void remaining(float *s,float dt){if(*s>0){*s-=dt;if(*s<0)*s=0;}}
int cnk_items_place(cnk_item_world *w,cnk_pickup_kind kind,
                    cnk_vec3 pos,float radius,cnk_weapon reward){
 if(!w||!point_ok(pos)||!isfinite(radius)||radius<0.1f||radius>15 ||
    w->pickup_count>=CNK_ITEM_MAX_PICKUPS ||
    (kind!=CNK_PICKUP_WUMPA&&kind!=CNK_PICKUP_CRATE)||
    (kind==CNK_PICKUP_CRATE && !reward_ok(reward))||
    (kind==CNK_PICKUP_WUMPA&&reward!=CNK_WEAPON_NONE))return -1;
 w->pickups[w->pickup_count++]=(cnk_item_pickup){pos,radius,0,kind,reward};
 return 0;
}
int cnk_items_use(cnk_item_world *w,uint32_t owner,const cnk_kart *kart,
                  uint32_t *events){
 if(!w||owner>=CNK_ITEM_MAX_RACERS||!kart||!events||
    !point_ok(kart->position)||!isfinite(kart->heading))return -1;
 cnk_item_racer *r=&w->racers[owner];
 if(!isfinite(w->clock_seconds)||w->clock_seconds<0.0f||
    !isfinite(w->candle_seconds)||w->candle_seconds<0.0f||
    w->clock_owner>=CNK_ITEM_MAX_RACERS||
    w->candle_owner>=CNK_ITEM_MAX_RACERS)return -1;
 if(r->held==CNK_WEAPON_NONE)return 0;
 /* While an opponent controls an effect, do not override it or consume the
  * second item. Multiple concurrent source clocks have not been recovered. */
 if(r->held==CNK_WEAPON_TROPY_CLOCK){
  if(w->clock_seconds>0.0f && w->clock_owner!=owner)return 0;
  w->clock_owner=owner;w->clock_seconds=3.0f;
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_CLOCK_ON;
  return 1;
 }
 if(r->held==CNK_WEAPON_CURSED_CANDLE){
  if(w->candle_seconds>0.0f && w->candle_owner!=owner)return 0;
  w->candle_owner=owner;w->candle_seconds=4.0f;
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_CANDLE_ON;
  return 1;
 }
 if(r->held==CNK_WEAPON_AKU_AKU){
  r->aku_seconds=3.0f; /* provisional: original duration is unknown */
  r->aku_tackle_cooldown=0.0f;
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_AKU_ON;
  return 1;
 }
 if(r->held==CNK_WEAPON_SHIELD){
  r->shield_seconds=2.5f; /* provisional: NOT extracted from original gameplay */
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_SHIELD_ON;
  return 1;
 }
 if(r->held==CNK_WEAPON_TURBO){
  r->turbo_seconds=1.4f;
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_TURBO_ON;
  return 1;
 }
 if(r->held==CNK_WEAPON_BOMB){
  uint32_t free_slot=CNK_ITEM_MAX_BOMBS;
  for(uint32_t i=0;i<CNK_ITEM_MAX_BOMBS;i++)if(!w->bombs[i].active){free_slot=i;break;}
  if(free_slot==CNK_ITEM_MAX_BOMBS)return 0;
  float dx=sinf(kart->heading),dz=cosf(kart->heading);
  cnk_vec3 origin=kart->position;origin.x+=dx*0.9f;origin.z+=dz*0.9f;
  if(!point_ok(origin))return -1;
  w->bombs[free_slot]=(cnk_item_bomb){origin,kart->heading,2.4f,owner,1};
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_BOMB_FIRED;
  return 1;
 }
 if(r->held==CNK_WEAPON_MISSILE){
  uint32_t slot=CNK_ITEM_MAX_MISSILES;
  for(uint32_t i=0;i<CNK_ITEM_MAX_MISSILES;i++)if(!w->missiles[i].active){slot=i;break;}
  if(slot==CNK_ITEM_MAX_MISSILES)return 0;
  cnk_vec3 pos=kart->position;
  pos.x+=sinf(kart->heading)*0.9f;
  pos.z+=cosf(kart->heading)*0.9f;
  if(!point_ok(pos))return -1;
  w->missiles[slot]=(cnk_item_missile){pos,kart->heading,3.25f,owner,UINT32_MAX,1};
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_MISSILE_FIRED;
  return 1;
 }
 if(r->held==CNK_WEAPON_TNT||r->held==CNK_WEAPON_OIL){
  uint32_t slot=CNK_ITEM_MAX_HAZARDS;
  for(uint32_t i=0;i<CNK_ITEM_MAX_HAZARDS;i++)if(!w->hazards[i].active){slot=i;break;}
  if(slot==CNK_ITEM_MAX_HAZARDS)return 0;
  cnk_vec3 pos=kart->position;
  pos.x-=sinf(kart->heading)*1.3f;
  pos.z-=cosf(kart->heading)*1.3f;
  if(!point_ok(pos))return -1;
  w->hazards[slot]=(cnk_item_hazard){pos,12.0f,owner,r->held,1};
  r->held=CNK_WEAPON_NONE;
  *events|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_HAZARD_DROPPED;
  return 1;
 }
 return -2;
}
int cnk_items_try_jack(cnk_item_world *w,uint32_t owner,
                       cnk_kart karts[CNK_ITEM_MAX_RACERS],
                       const uint8_t eligible[CNK_ITEM_MAX_RACERS],
                       uint32_t count,uint32_t events[CNK_ITEM_MAX_RACERS]){
 if(!w||!karts||!eligible||!events||count<1||count>CNK_ITEM_MAX_RACERS||
    owner>=count)return -1;
 if(w->racers[owner].held!=CNK_WEAPON_KART_JACKING)return 0;
 /* Validate ALL candidate poses before mutation. Invalid geometry cannot
  * spend an item or produce a half-swapped pair. */
 for(uint32_t i=0;i<count;i++)if(!point_ok(karts[i].position)||
   !isfinite(karts[i].heading)||!isfinite(karts[i].speed))return -2;
 if(count<2 || !eligible[owner])return 0;
 const cnk_kart *self=&karts[owner];
 float forward_x=sinf(self->heading),forward_z=cosf(self->heading);
 uint32_t nearest=count;
 float best=18.0f*18.0f;
 for(uint32_t i=0;i<count;i++)if(i!=owner&&eligible[i]){
  float dx=karts[i].position.x-self->position.x;
  float dz=karts[i].position.z-self->position.z;
  float dist2=dx*dx+dz*dz;
  if(dist2<=1.0f || dist2>=best ||
     fabsf(karts[i].position.y-self->position.y)>2.0f)continue;
  if(dx*forward_x+dz*forward_z <= 0.80f*sqrtf(dist2))continue;
  best=dist2;nearest=i;
 }
 if(nearest==count)return 0;
 /* Swap *pose* only. Neither time/lap/position in standings nor item stacks
  * can be transferred. A session caller must re-prime checkpoint-side signs. */
 cnk_vec3 pos=karts[owner].position;
 float heading=karts[owner].heading;
 karts[owner].position=karts[nearest].position;
 karts[owner].heading=karts[nearest].heading;
 karts[nearest].position=pos;
 karts[nearest].heading=heading;
 w->racers[owner].held=CNK_WEAPON_NONE;
 events[owner]|=CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_KART_JACK;
 return (int)nearest+1;
}
int cnk_items_ai_use(cnk_item_world *w,uint32_t owner,
                     const cnk_kart karts[CNK_ITEM_MAX_RACERS],
                     const uint8_t movable[CNK_ITEM_MAX_RACERS],
                     uint32_t count,uint32_t *events){
 if(!w||!karts||!movable||!events||count>CNK_ITEM_MAX_RACERS||
    owner>=count||owner==0)return -1;
 if(!movable[owner])return 0;
 const cnk_kart *self=&karts[owner];
 if(!point_ok(self->position)||!isfinite(self->heading)||!isfinite(self->speed))return -2;
 if(w->racers[owner].held==CNK_WEAPON_TROPY_CLOCK ||
    w->racers[owner].held==CNK_WEAPON_CURSED_CANDLE){
  if(count<2)return 0;
  return cnk_items_use(w,owner,self,events);
 }
 if(w->racers[owner].held==CNK_WEAPON_AKU_AKU){
  if(w->racers[owner].aku_seconds>0.0f)return 0;
  return cnk_items_use(w,owner,self,events);
 }
 if(w->racers[owner].held==CNK_WEAPON_SHIELD){
  if(w->racers[owner].shield_seconds>0)return 0;
  return cnk_items_use(w,owner,self,events);
 }
 if(w->racers[owner].held==CNK_WEAPON_TURBO){
  if(self->speed<4.0f)return 0;
  return cnk_items_use(w,owner,self,events);
 }
 if(w->racers[owner].held==CNK_WEAPON_TNT || w->racers[owner].held==CNK_WEAPON_OIL){
  /* Only drop a rear hazard if another kart is behind the owner. */
  float dx=sinf(self->heading),dz=cosf(self->heading);
  for(uint32_t i=0;i<count;i++)if(i!=owner&&movable[i]){
   if(!point_ok(karts[i].position))return -2;
   float x=karts[i].position.x-self->position.x;
   float z=karts[i].position.z-self->position.z;
   float d2=x*x+z*z;
   if(d2>1.0f&&d2<49.0f && (x*dx+z*dz)<-0.65f*sqrtf(d2) &&
      fabsf(karts[i].position.y-self->position.y)<2.0f)
    return cnk_items_use(w,owner,self,events);
  }
  return 0;
 }
 if(w->racers[owner].held!=CNK_WEAPON_BOMB &&
    w->racers[owner].held!=CNK_WEAPON_MISSILE)return 0;
 /* Fire only with a live rival inside a conservative forward cone, without
  * inventing opponent tactics or shooting the bomb off the track blindly. */
 float dx=sinf(self->heading),dz=cosf(self->heading);
 for(uint32_t i=0;i<count;i++)if(i!=owner&&movable[i]){
  if(!point_ok(karts[i].position))return -2;
  float x=karts[i].position.x-self->position.x;
  float z=karts[i].position.z-self->position.z;
  float d2=x*x+z*z;
  if(d2>0.25f&&d2<36.0f && (x*dx+z*dz)>0.84f*sqrtf(d2) &&
     fabsf(karts[i].position.y-self->position.y)<2.0f)
   return cnk_items_use(w,owner,self,events);
 }
 return 0;
}
int cnk_items_tune(const cnk_item_world *w,uint32_t racer,
                   const cnk_drive_tuning *base,cnk_drive_tuning *out){
 if(!w||!base||!out||racer>=CNK_ITEM_MAX_RACERS ||
    !isfinite(w->racers[racer].turbo_seconds)||
    !isfinite(w->racers[racer].stun_seconds)||
    !isfinite(w->racers[racer].oil_seconds)||
    !isfinite(w->racers[racer].shield_seconds)||
    !isfinite(w->racers[racer].aku_seconds)||
    !isfinite(w->racers[racer].aku_tackle_cooldown)||
    w->racers[racer].wumpa>10u||
    w->racers[racer].turbo_seconds<0 || w->racers[racer].stun_seconds<0 ||
    w->racers[racer].oil_seconds<0 ||
    w->racers[racer].shield_seconds<0 ||
    w->racers[racer].aku_seconds<0 ||
    w->racers[racer].aku_tackle_cooldown<0 ||
    !isfinite(w->clock_seconds)||w->clock_seconds<0.0f||
    !isfinite(w->candle_seconds)||w->candle_seconds<0.0f||
    w->clock_owner>=CNK_ITEM_MAX_RACERS||
    w->candle_owner>=CNK_ITEM_MAX_RACERS||
    cnk_drive_validate(base))return -1;
 cnk_drive_tuning tmp=*base;
 /* Zeebo LANG item help explicitly says retaining 10 Wumpa fruits provides
  * a speed boost. These numeric modifiers remain provisional. */
 if(w->racers[racer].wumpa==10u){
  tmp.max_speed+=1.5f;
  tmp.acceleration+=2.0f;
 }
 if(w->racers[racer].aku_seconds>0.0f){
  tmp.max_speed+=3.0f;
  tmp.acceleration+=9.0f;
 }
 if(w->racers[racer].turbo_seconds>0){
  tmp.max_speed+=3.5f;
  tmp.acceleration+=13.0f;
 }
 if(w->racers[racer].stun_seconds>0){
  tmp.max_speed*=0.45f;
  tmp.acceleration*=0.2f;
 }
 if(w->racers[racer].oil_seconds>0){
  tmp.max_speed*=0.78f;
  tmp.turn_rate*=0.55f;
  tmp.wheel_response*=0.4f;
 }
 if(w->clock_seconds>0.0f && w->clock_owner!=racer){
  /* Clock slows driving, not race clocks or checkpoints. Numerical tuning
   * is EXPLICITLY provisional; the original text supplies no coefficient. */
  tmp.max_speed*=0.45f;
  tmp.acceleration*=0.55f;
  tmp.min_ai_speed=fminf(tmp.min_ai_speed,tmp.max_speed);
 }
 /* Stun/oil in combination with the clock may reduce top speed below the
  * ordinary AI floor; keep the tuning valid without changing plain races. */
 tmp.min_ai_speed=fminf(tmp.min_ai_speed,tmp.max_speed);
 if(cnk_drive_validate(&tmp))return -2;
 *out=tmp;return 0;
}
/* Source LANG describes losing some Wumpas when hit. Exact amount unknown;
 * this provisional value is confined here and never used for shielded hits. */
static void lose_wumpa(cnk_item_world *w,uint32_t index,uint32_t *events){
 uint8_t *count=&w->racers[index].wumpa;
 if(*count){*count=(uint8_t)(*count>2u?*count-2u:0u);
  *events|=CNK_ITEM_EVENT_WUMPA_LOST;}
}
/* Stop a flying item at the earliest verified triangle intersection.
 * Source materials currently have no authenticated collidability flags,
 * so this path is explicitly optional. The segment is raised slightly from
 * the source collision plane to detect upright walls rather than the road. */
static int projectile_wall_fraction(const cnk_collision_grid *walls,float steepness,
                                    cnk_vec3 a,cnk_vec3 b,float *fraction){
 *fraction=1.0f;
 if(!walls)return 0;
 a.y+=0.4f;b.y+=0.4f;
 float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z;
 if(dx*dx+dy*dy+dz*dz<1.0e-12f)return 0;
 cnk_segment_hit hit={0};
 int rc=cnk_collision_segment_cast_steep(walls,a,b,steepness,&hit);
 if(rc==-2)return 0;
 if(rc<0)return -1;
 if(!isfinite(hit.fraction)||hit.fraction<0.0f||hit.fraction>1.0f)return -1;
 *fraction=hit.fraction;
 return 1;
}
static int items_step_impl(cnk_item_world *w,cnk_kart karts[CNK_ITEM_MAX_RACERS],
                   const cnk_vec3 before[CNK_ITEM_MAX_RACERS],
                   const uint8_t movable[CNK_ITEM_MAX_RACERS],
                   uint32_t count,float dt,uint32_t events[CNK_ITEM_MAX_RACERS],
                   const cnk_collision_grid *walls,float steepness){
 if(!w||!karts||!before||!movable||!events||!count||count>CNK_ITEM_MAX_RACERS ||
    !isfinite(dt)||dt<=0||dt>0.10f||w->pickup_count>CNK_ITEM_MAX_PICKUPS)return -1;
 if(!isfinite(w->clock_seconds)||w->clock_seconds<0.0f||
    !isfinite(w->candle_seconds)||w->candle_seconds<0.0f||
    w->clock_owner>=CNK_ITEM_MAX_RACERS||
    w->candle_owner>=CNK_ITEM_MAX_RACERS)return -1;
 for(uint32_t i=0;i<count;i++)if(!point_ok(karts[i].position)||!point_ok(before[i])||
        !isfinite(karts[i].speed)||!isfinite(karts[i].heading)||
        w->racers[i].wumpa>10u||!isfinite(w->racers[i].shield_seconds)||
        w->racers[i].shield_seconds<0.0f ||
        !isfinite(w->racers[i].aku_seconds)||w->racers[i].aku_seconds<0.0f||
        !isfinite(w->racers[i].aku_tackle_cooldown)||
        w->racers[i].aku_tackle_cooldown<0.0f)return -2;
 for(uint32_t p=0;p<w->pickup_count;p++)if(!point_ok(w->pickups[p].position)||
        !isfinite(w->pickups[p].radius)||w->pickups[p].radius<=0)return -3;
 /* Reject malformed live actors before making any changes to pickups,
  * health statuses or karts in this tick. */
 for(uint32_t b=0;b<CNK_ITEM_MAX_BOMBS;b++)if(w->bombs[b].active &&
      (!point_ok(w->bombs[b].position)||!isfinite(w->bombs[b].heading)||
       !isfinite(w->bombs[b].lifetime)||w->bombs[b].owner>=count))return -5;
 for(uint32_t b=0;b<CNK_ITEM_MAX_MISSILES;b++)if(w->missiles[b].active &&
      (!point_ok(w->missiles[b].position)||!isfinite(w->missiles[b].heading)||
       !isfinite(w->missiles[b].lifetime)||w->missiles[b].owner>=count ||
       (w->missiles[b].target!=UINT32_MAX && w->missiles[b].target>=count)))return -5;
 for(uint32_t b=0;b<CNK_ITEM_MAX_HAZARDS;b++)if(w->hazards[b].active &&
      (!point_ok(w->hazards[b].position)||!isfinite(w->hazards[b].lifetime)||
       w->hazards[b].owner>=count ||
       (w->hazards[b].kind!=CNK_WEAPON_TNT&&w->hazards[b].kind!=CNK_WEAPON_OIL)))return -5;
 /* Treat all pickups as one chronological stream: the same kart can
  * legitimately collect several Wumpa fruits, but receives the FIRST crate
  * it touches rather than whichever crate has the lowest storage index.
  * Every candidate is geometric and bounded (32 pickups * 6 racers). */
 cnk_pickup_touch touches[CNK_ITEM_MAX_PICKUPS*CNK_ITEM_MAX_RACERS];
 uint32_t touch_count=0;
 for(uint32_t p=0;p<w->pickup_count;p++){
  cnk_item_pickup *box=&w->pickups[p];
  if(box->unavailable_seconds>0){remaining(&box->unavailable_seconds,dt);continue;}
  if((box->kind!=CNK_PICKUP_WUMPA&&box->kind!=CNK_PICKUP_CRATE)||
     (box->kind==CNK_PICKUP_CRATE&&!reward_ok(box->reward)))return -4;
  for(uint32_t i=0;i<count;i++){
   if(!movable[i])continue;
   float t=0.0f;
   if(!first_contact_xz(before[i],karts[i].position,
                         box->position,box->radius,&t))continue;
   float y=before[i].y+(karts[i].position.y-before[i].y)*t;
   if(fabsf(y-box->position.y)>2.0f)continue;
   touches[touch_count++]=(cnk_pickup_touch){t,p,i};
  }
 }
 qsort(touches,touch_count,sizeof(touches[0]),pickup_touch_compare);
 for(uint32_t n=0;n<touch_count;n++){
  const cnk_pickup_touch *touch=&touches[n];
  cnk_item_pickup *box=&w->pickups[touch->pickup];
  cnk_item_racer *r=&w->racers[touch->racer];
  if(box->unavailable_seconds>0 ||
     (box->kind==CNK_PICKUP_CRATE&&r->held!=CNK_WEAPON_NONE))continue;
  if(box->kind==CNK_PICKUP_WUMPA){
   if(r->wumpa<10u)r->wumpa++;
   events[touch->racer]|=CNK_ITEM_EVENT_WUMPA;
  }else{
   r->held=box->reward;
   events[touch->racer]|=CNK_ITEM_EVENT_WEAPON;
  }
  box->unavailable_seconds=5.0f; /* provisional cooldown */
 }
 /* Relative segment sweep against MOVING karts prevents missed bomb hits. */
 for(uint32_t b=0;b<CNK_ITEM_MAX_BOMBS;b++){
  cnk_item_bomb *bomb=&w->bombs[b];
  if(!bomb->active)continue;
  if(!point_ok(bomb->position)||!isfinite(bomb->heading)||!isfinite(bomb->lifetime)||
     bomb->owner>=count)return -5;
  cnk_vec3 prev=bomb->position;
  bomb->position.x+=sinf(bomb->heading)*11.0f*dt;
  bomb->position.z+=cosf(bomb->heading)*11.0f*dt;
  bomb->lifetime-=dt;
  if(!point_ok(bomb->position))return -6;
  float wall_frac=1.0f;
  int wall_hit=projectile_wall_fraction(walls,steepness,prev,bomb->position,&wall_frac);
  if(wall_hit<0)return -7;
  if(wall_hit){
   bomb->position.x=prev.x+(bomb->position.x-prev.x)*wall_frac;
   bomb->position.z=prev.z+(bomb->position.z-prev.z)*wall_frac;
  }
  /* Resolve one flying bomb against the FIRST racer touched, not the
   * lowest-index racer that happens to overlap its swept path. */
  uint32_t first_racer=count;
  float first_time=2.0f;
  for(uint32_t i=0;i<count;i++){
   if(i==bomb->owner || !movable[i])continue;
   cnk_vec3 rel_start={prev.x-before[i].x,0,prev.z-before[i].z};
   cnk_vec3 rel_end={bomb->position.x-(before[i].x+
                       (karts[i].position.x-before[i].x)*wall_frac),0,
                     bomb->position.z-(before[i].z+
                       (karts[i].position.z-before[i].z)*wall_frac)};
   float contact=0.0f;
   if(!first_contact_xz(rel_start,rel_end,(cnk_vec3){0},0.65f,&contact))continue;
   float y=before[i].y+(karts[i].position.y-before[i].y)*wall_frac*contact;
   if(fabsf(y-bomb->position.y)>2.0f)continue;
   if(contact<first_time){first_time=contact;first_racer=i;}
  }
  if(first_racer<count){
   if(w->racers[first_racer].shield_seconds>0.0f){
    events[first_racer]|=CNK_ITEM_EVENT_SHIELD_BLOCK;
   }else{
    karts[first_racer].speed*=0.35f;
    w->racers[first_racer].stun_seconds=0.70f;
    events[first_racer]|=CNK_ITEM_EVENT_HIT;
    lose_wumpa(w,first_racer,&events[first_racer]);
   }
   bomb->active=0;
  }
  if(bomb->active && wall_hit){
   bomb->active=0;
   events[bomb->owner]|=CNK_ITEM_EVENT_BOMB_WALL;
  }
  if(bomb->lifetime<=0)bomb->active=0;
 }
 /* Homing missiles: bounded angular velocity, moving-kart relative
  * sweep, nearest target ahead. All constants are EXPERIMENTAL. */
 for(uint32_t b=0;b<CNK_ITEM_MAX_MISSILES;b++){
  cnk_item_missile *m=&w->missiles[b];
  if(!m->active)continue;
  cnk_vec3 prev=m->position;
  if(m->target!=UINT32_MAX){
   cnk_vec3 delta={karts[m->target].position.x-m->position.x,0,
                   karts[m->target].position.z-m->position.z};
   if(!movable[m->target]||delta.x*delta.x+delta.z*delta.z>80.0f*80.0f)
    m->target=UINT32_MAX;
  }
  if(m->target==UINT32_MAX){
   float nearest=24.0f*24.0f;
   for(uint32_t i=0;i<count;i++)if(i!=m->owner&&movable[i]){
    float dx=karts[i].position.x-m->position.x;
    float dz=karts[i].position.z-m->position.z;
    float distance=dx*dx+dz*dz;
    if(distance<nearest && fabsf(karts[i].position.y-m->position.y)<2.0f &&
       dx*sinf(m->heading)+dz*cosf(m->heading)>0.30f*sqrtf(distance)){
     nearest=distance;m->target=i;
    }
   }
  }
  if(m->target!=UINT32_MAX){
   float dx=karts[m->target].position.x-m->position.x;
   float dz=karts[m->target].position.z-m->position.z;
   float turn=wrapped_angle(atan2f(dx,dz)-m->heading);
   float limit=3.0f*dt;
   if(turn>limit)turn=limit;
   if(turn< -limit)turn=-limit;
   m->heading=wrapped_angle(m->heading+turn);
  }
  m->position.x+=sinf(m->heading)*15.0f*dt;
  m->position.z+=cosf(m->heading)*15.0f*dt;
  m->lifetime-=dt;
  if(!point_ok(m->position))return -6;
  float wall_frac=1.0f;
  int wall_hit=projectile_wall_fraction(walls,steepness,prev,m->position,&wall_frac);
  if(wall_hit<0)return -7;
  if(wall_hit){
   m->position.x=prev.x+(m->position.x-prev.x)*wall_frac;
   m->position.z=prev.z+(m->position.z-prev.z)*wall_frac;
  }
  /* Nearest actual swept impact, independent of racer slot ordering. */
  uint32_t first_racer=count;
  float first_time=2.0f;
  for(uint32_t i=0;i<count;i++)if(i!=m->owner&&movable[i]){
   cnk_vec3 rel0={prev.x-before[i].x,0,prev.z-before[i].z};
   cnk_vec3 rel1={m->position.x-(before[i].x+
                    (karts[i].position.x-before[i].x)*wall_frac),0,
                  m->position.z-(before[i].z+
                    (karts[i].position.z-before[i].z)*wall_frac)};
   float contact=0.0f;
   if(!first_contact_xz(rel0,rel1,(cnk_vec3){0},0.75f,&contact))continue;
   float y=before[i].y+(karts[i].position.y-before[i].y)*wall_frac*contact;
   if(fabsf(y-m->position.y)>2.0f)continue;
   if(contact<first_time){first_time=contact;first_racer=i;}
  }
  if(first_racer<count){
   if(w->racers[first_racer].shield_seconds>0.0f){
    events[first_racer]|=CNK_ITEM_EVENT_SHIELD_BLOCK;
   }else{
    karts[first_racer].speed*=0.30f;
    w->racers[first_racer].stun_seconds=1.0f;
    events[first_racer]|=CNK_ITEM_EVENT_HIT|CNK_ITEM_EVENT_MISSILE_HIT;
    lose_wumpa(w,first_racer,&events[first_racer]);
   }
   m->active=0;
  }
  if(m->active && wall_hit){
   m->active=0;
   events[m->owner]|=CNK_ITEM_EVENT_MISSILE_WALL;
  }
  if(m->lifetime<=0)m->active=0;
 }

 /* Source LANG confirms Aku-Aku allows tackling opponents. This is a
  * conservative provisional contact: swept relative XZ intersection, forward
  * approach and vertical separation, never a teleport or checkpoint grant.
  * Temporarily reduce the victim's speed; source knockback animation/physics
  * remain unrecovered. Each attacker has a cooldown to prevent 60Hz stacking. */
 for(uint32_t a=0;a<count;a++)if(movable[a] &&
     w->racers[a].aku_seconds>0.0f &&
     w->racers[a].aku_tackle_cooldown<=0.0f &&
     karts[a].speed>3.0f){
  float first=2.0f;uint32_t target=count;
  for(uint32_t b=0;b<count;b++)if(b!=a && movable[b]){
   cnk_vec3 relative0={before[a].x-before[b].x,0,
                        before[a].z-before[b].z};
   cnk_vec3 relative1={karts[a].position.x-karts[b].position.x,0,
                        karts[a].position.z-karts[b].position.z};
   float contact=0.0f;
   if(!first_contact_xz(relative0,relative1,(cnk_vec3){0},0.9f,&contact))continue;
   float ay=before[a].y+(karts[a].position.y-before[a].y)*contact;
   float by=before[b].y+(karts[b].position.y-before[b].y)*contact;
   if(fabsf(ay-by)>1.7f)continue;
   float approach_x=before[b].x-before[a].x;
   float approach_z=before[b].z-before[a].z;
   if(approach_x*sinf(karts[a].heading)+
      approach_z*cosf(karts[a].heading)<-0.1f)continue;
   if(contact<first){first=contact;target=b;}
  }
  if(target<count){
   w->racers[a].aku_tackle_cooldown=0.5f;
   events[a]|=CNK_ITEM_EVENT_AKU_TACKLE;
   if(w->racers[target].shield_seconds>0.0f)
    events[target]|=CNK_ITEM_EVENT_SHIELD_BLOCK;
   else{
    karts[target].speed*=0.38f;
    w->racers[target].stun_seconds=0.4f;
    events[target]|=CNK_ITEM_EVENT_HIT;
    lose_wumpa(w,target,&events[target]);
   }
  }
 }
 /* Rear traps remain stationary, unlike flying projectiles. A racer cannot
  * trigger their own traps. The XZ sweep handles high-speed crossings. */
 for(uint32_t b=0;b<CNK_ITEM_MAX_HAZARDS;b++){
  cnk_item_hazard *h=&w->hazards[b];
  if(!h->active)continue;
  uint32_t first_racer=count;
  float first_time=2.0f;
  for(uint32_t i=0;i<count;i++)if(i!=h->owner&&movable[i]){
   float contact=0;
   if(!first_contact_xz(before[i],karts[i].position,
                         h->position,0.85f,&contact))continue;
   float y=before[i].y+(karts[i].position.y-before[i].y)*contact;
   if(fabsf(y-h->position.y)>2.0f)continue;
   if(contact<first_time){first_time=contact;first_racer=i;}
  }
  if(first_racer<count){
   int blocked=w->racers[first_racer].shield_seconds>0.0f;
   if(blocked){
    events[first_racer]|=CNK_ITEM_EVENT_SHIELD_BLOCK;
   }else if(h->kind==CNK_WEAPON_TNT){
    karts[first_racer].speed*=0.2f;
    w->racers[first_racer].stun_seconds=1.3f;
    events[first_racer]|=CNK_ITEM_EVENT_HIT|CNK_ITEM_EVENT_TNT_HIT;
   }else{
    karts[first_racer].speed*=0.8f;
    w->racers[first_racer].oil_seconds=1.25f;
    events[first_racer]|=CNK_ITEM_EVENT_HIT|CNK_ITEM_EVENT_OIL_HIT;
   }
   if(!blocked)lose_wumpa(w,first_racer,&events[first_racer]);
   h->active=0;
  }
  if(h->active){h->lifetime-=dt;if(h->lifetime<=0)h->active=0;}
 }
 for(uint32_t i=0;i<count;i++){
  if(!movable[i])continue;
  cnk_item_racer *r=&w->racers[i];
  int boosted=r->turbo_seconds>0;
  remaining(&r->turbo_seconds,dt);
  remaining(&r->stun_seconds,dt);
  remaining(&r->oil_seconds,dt);
  int aku_active=r->aku_seconds>0.0f;
  remaining(&r->aku_seconds,dt);
  remaining(&r->aku_tackle_cooldown,dt);
  if(aku_active && r->aku_seconds==0.0f)
   events[i]|=CNK_ITEM_EVENT_AKU_OFF;
  int shielded=r->shield_seconds>0.0f;
  remaining(&r->shield_seconds,dt);
  if(shielded && r->shield_seconds==0.0f)
   events[i]|=CNK_ITEM_EVENT_SHIELD_OFF;
  if(boosted && r->turbo_seconds==0)events[i]|=CNK_ITEM_EVENT_TURBO_OFF;
 }
 if(w->clock_seconds>0.0f){
  remaining(&w->clock_seconds,dt);
  if(w->clock_seconds==0.0f && w->clock_owner<count)
   events[w->clock_owner]|=CNK_ITEM_EVENT_CLOCK_OFF;
 }
 if(w->candle_seconds>0.0f){
  remaining(&w->candle_seconds,dt);
  if(w->candle_seconds==0.0f && w->candle_owner<count)
   events[w->candle_owner]|=CNK_ITEM_EVENT_CANDLE_OFF;
 }
 return 0;
}

int cnk_items_step(cnk_item_world *world,cnk_kart karts[CNK_ITEM_MAX_RACERS],
                   const cnk_vec3 before[CNK_ITEM_MAX_RACERS],
                   const uint8_t movable[CNK_ITEM_MAX_RACERS],
                   uint32_t count,float dt,uint32_t events[CNK_ITEM_MAX_RACERS]){
 if(!world||!karts||!before||!movable||!events)return -1;
 /* The ordinary non-wall path must have the same transactional guarantee
  * as the wall-enabled path. A rejected source actor, malformed pickup or
  * invalid dt cannot leave some pickups consumed or a kart half-stunned. */
 cnk_item_world candidate=*world;
 cnk_kart next_karts[CNK_ITEM_MAX_RACERS];
 uint32_t next_events[CNK_ITEM_MAX_RACERS];
 memcpy(next_karts,karts,sizeof(next_karts));
 memcpy(next_events,events,sizeof(next_events));
 int rc=items_step_impl(&candidate,next_karts,before,movable,count,dt,
                        next_events,NULL,0.0f);
 if(rc)return rc;
 *world=candidate;
 memcpy(karts,next_karts,sizeof(next_karts));
 memcpy(events,next_events,sizeof(next_events));
 return 0;
}
int cnk_items_step_world(cnk_item_world *world,cnk_kart karts[CNK_ITEM_MAX_RACERS],
                   const cnk_vec3 before[CNK_ITEM_MAX_RACERS],
                   const uint8_t movable[CNK_ITEM_MAX_RACERS],
                   uint32_t count,float dt,uint32_t events[CNK_ITEM_MAX_RACERS],
                   const cnk_collision_grid *walls,float max_abs_normal_y){
 if(!world||!karts||!events||!walls || !walls->mesh || !walls->offsets ||
    !walls->indices || !isfinite(max_abs_normal_y)||
    max_abs_normal_y<0.0f||max_abs_normal_y>1.0f)return -1;
 /* A malformed source collision grid must not partly modify inventory,
  * launch projectiles or stun a kart before the frame reports failure. */
 cnk_item_world next=*world;
 cnk_kart next_karts[CNK_ITEM_MAX_RACERS];
 uint32_t next_events[CNK_ITEM_MAX_RACERS];
 memcpy(next_karts,karts,sizeof(next_karts));
 memcpy(next_events,events,sizeof(next_events));
 int rc=items_step_impl(&next,next_karts,before,movable,count,dt,next_events,
                        walls,max_abs_normal_y);
 if(rc)return rc;
 *world=next;
 memcpy(karts,next_karts,sizeof(next_karts));
 memcpy(events,next_events,sizeof(next_events));
 return 0;
}
