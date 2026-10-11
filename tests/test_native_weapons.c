#include "cnk_items.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static cnk_kart karts[CNK_ITEM_MAX_RACERS];
static cnk_vec3 before[CNK_ITEM_MAX_RACERS];
static uint8_t can_move[CNK_ITEM_MAX_RACERS]={1,1,1};
static uint32_t evt[CNK_ITEM_MAX_RACERS];
static void advance(cnk_item_world *w){
 for(int i=0;i<3;i++)before[i]=karts[i].position;
 assert(cnk_items_step(w,karts,before,can_move,3,1.0f/60.0f,evt)==0);
}
static void setup(void){
 memset(karts,0,sizeof(karts));memset(evt,0,sizeof(evt));
 karts[0].position=(cnk_vec3){0,0,0};karts[0].heading=0;
 karts[1].position=(cnk_vec3){3,0,12};karts[1].speed=9;
 karts[2].position=(cnk_vec3){-50,0,-50};
}
static void tested_missile(void){
 cnk_item_world w={0};uint32_t fire=0;setup();
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){0},1,CNK_WEAPON_MISSILE));
 w.racers[0].held=CNK_WEAPON_MISSILE;
 assert(cnk_items_use(&w,0,&karts[0],&fire)==1);
 assert((fire&(CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_MISSILE_FIRED))==
             (CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_MISSILE_FIRED));
 assert(w.missiles[0].active && w.missiles[0].target==UINT32_MAX);
 advance(&w);
 assert(w.missiles[0].target==1);
 assert(w.missiles[0].heading>0.0f); /* actual guidance, not straight-line sprite */
 for(int i=0;i<180 && !(evt[1]&CNK_ITEM_EVENT_MISSILE_HIT);i++)advance(&w);
 assert(evt[1]&CNK_ITEM_EVENT_MISSILE_HIT);
 assert(evt[1]&CNK_ITEM_EVENT_HIT);
 assert(karts[1].speed<9.0f && w.racers[1].stun_seconds>0);
 assert(!w.missiles[0].active && !(evt[0]&CNK_ITEM_EVENT_HIT));
 /* A spent missile must never damage the same kart twice. */
 float saved_speed=karts[1].speed;
 advance(&w);assert(karts[1].speed==saved_speed);
 /* Full pool has no item loss. */
 w.racers[0].held=CNK_WEAPON_MISSILE;
 for(unsigned j=0;j<CNK_ITEM_MAX_MISSILES;j++)w.missiles[j].active=1;
 fire=0;assert(!cnk_items_use(&w,0,&karts[0],&fire));
 assert(w.racers[0].held==CNK_WEAPON_MISSILE && !fire);
 /* A distant target is not falsely acquired. No hit if y is out of range. */
 memset(&w,0,sizeof(w));setup();
 karts[1].position=(cnk_vec3){0,12,10};
 w.racers[0].held=CNK_WEAPON_MISSILE;
 assert(cnk_items_use(&w,0,&karts[0],&fire)==1);
 for(int j=0;j<210;j++)advance(&w);
 assert(!(evt[1]&CNK_ITEM_EVENT_HIT));
 assert(!w.missiles[0].active);
}
static void tested_hazard(cnk_weapon kind){
 cnk_item_world w={0};setup();
 uint32_t fire=0;
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){1,0,1},1,kind));
 w.racers[0].held=kind;
 assert(cnk_items_use(&w,0,&karts[0],&fire)==1);
 assert((fire&(CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_HAZARD_DROPPED))==
              (CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_HAZARD_DROPPED));
 assert(w.hazards[0].active && w.hazards[0].kind==kind);
 assert(w.hazards[0].position.z<karts[0].position.z);
 advance(&w);assert(!(evt[0]&CNK_ITEM_EVENT_HIT)); /* no self-hit */
 karts[1].position=(cnk_vec3){0,0,-3.5f};
 before[1]=karts[1].position;
 karts[1].position=(cnk_vec3){0,0,0.0f};
 before[0]=karts[0].position;before[2]=karts[2].position;
 assert(!cnk_items_step(&w,karts,before,can_move,3,1.0f/60.0f,evt));
 assert(evt[1]&CNK_ITEM_EVENT_HIT);
 assert(!w.hazards[0].active);
 cnk_drive_tuning base={0},out={0};cnk_drive_default_tuning(&base);
 assert(!cnk_items_tune(&w,1,&base,&out));
 if(kind==CNK_WEAPON_OIL){
  assert(evt[1]&CNK_ITEM_EVENT_OIL_HIT);
  assert(w.racers[1].oil_seconds>0 && out.turn_rate<base.turn_rate &&
         out.wheel_response<base.wheel_response);
 }else{
  assert(evt[1]&CNK_ITEM_EVENT_TNT_HIT);
  assert(w.racers[1].stun_seconds>0 && out.max_speed<base.max_speed);
 }
 /* Physical effect expires instead of being permanent. */
 for(int j=0;j<120;j++)advance(&w);
 assert(!cnk_items_tune(&w,1,&base,&out));
 assert(out.max_speed==base.max_speed && out.turn_rate==base.turn_rate);
 /* Stationary trap must expire after TTL with no other kart contact. */
 memset(&w,0,sizeof(w));setup();
 w.racers[0].held=kind;
 assert(cnk_items_use(&w,0,&karts[0],&fire)==1);
 for(int j=0;j<725;j++)advance(&w);
 assert(!w.hazards[0].active);
 /* Pool exhausted: equipped item is preserved. */
 for(unsigned j=0;j<CNK_ITEM_MAX_HAZARDS;j++)w.hazards[j].active=1;
 w.racers[0].held=kind;fire=0;
 assert(cnk_items_use(&w,0,&karts[0],&fire)==0);
 assert(w.racers[0].held==kind && fire==0);
}
static void tested_validation_and_ai(void){
 cnk_item_world w={0};setup();
 cnk_kart a[CNK_ITEM_MAX_RACERS]={0};uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1,1};
 uint32_t fire=0;
 a[1].position=(cnk_vec3){0,0,0};a[1].heading=0;
 a[0].position=(cnk_vec3){0,0,5};
 w.racers[1].held=CNK_WEAPON_MISSILE;
 assert(cnk_items_ai_use(&w,1,a,movable,3,&fire)==1);
 assert(fire&CNK_ITEM_EVENT_MISSILE_FIRED);
 assert(w.missiles[0].active && w.racers[1].held==CNK_WEAPON_NONE);
 memset(&w,0,sizeof(w));fire=0;
 w.racers[1].held=CNK_WEAPON_TNT;
 assert(!cnk_items_ai_use(&w,1,a,movable,3,&fire));
 a[0].position=(cnk_vec3){0,0,-4};
 assert(cnk_items_ai_use(&w,1,a,movable,3,&fire)==1);
 assert(w.hazards[0].active);
 memset(&w,0,sizeof(w));
 w.missiles[0].active=1;w.missiles[0].owner=0;w.missiles[0].target=UINT32_MAX;
 w.missiles[0].position.x=NAN;
 cnk_item_world copy=w;
 cnk_kart original=karts[1];
 for(int i=0;i<3;i++)before[i]=karts[i].position;
 assert(cnk_items_step(&w,karts,before,can_move,3,1.0f/60,evt)<0);
 assert(memcmp(&w,&copy,sizeof(w))==0);
 assert(memcmp(&karts[1],&original,sizeof(original))==0);
 memset(&w,0,sizeof(w));w.hazards[0].active=1;w.hazards[0].kind=CNK_WEAPON_BOMB;
 assert(cnk_items_step(&w,karts,before,can_move,3,1.0f/60,evt)<0);
 memset(&w,0,sizeof(w));
 assert(cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){0},1,(cnk_weapon)77)<0);
}
int main(void){
 tested_missile();tested_hazard(CNK_WEAPON_TNT);
 tested_hazard(CNK_WEAPON_OIL);tested_validation_and_ai();
 puts("PASS: homing missile steering/hit/expiry, TNT/oil traps, AI, depleted pools, atomic invalid actors");
 return 0;
}
