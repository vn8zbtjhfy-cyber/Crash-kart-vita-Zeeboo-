#include "cnk_item_visual.h"
#include <math.h>
#include <string.h>
static int valid(cnk_vec3 p,float r){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z)&&isfinite(r)&&r>0.0f&&r<=100.0f;}
static int append(cnk_item_visual_frame *frame,cnk_vec3 p,float size,cnk_item_sprite s){
 if(!valid(p,size)||frame->count>=CNK_ITEM_VISUAL_CAPACITY)return -1;
 frame->actors[frame->count++]=(cnk_item_visual){p,size,s};return 0;
}
int cnk_item_visual_collect(const cnk_item_world *world,cnk_item_visual_frame *out){
 if(!world||!out||world->pickup_count>CNK_ITEM_MAX_PICKUPS)return -1;
 cnk_item_visual_frame tmp={0};
 for(uint32_t i=0;i<world->pickup_count;i++){
  const cnk_item_pickup *p=&world->pickups[i];
  if(!valid(p->position,p->radius)||!isfinite(p->unavailable_seconds)||
      p->unavailable_seconds<0.0f ||
      (p->kind!=CNK_PICKUP_WUMPA&&p->kind!=CNK_PICKUP_CRATE))return -2;
  if(p->unavailable_seconds>0.0f)continue;
  if(append(&tmp,p->position,p->radius,p->kind==CNK_PICKUP_WUMPA?
            CNK_ITEM_SPRITE_WUMPA:CNK_ITEM_SPRITE_CRATE))return -3;
 }
 for(uint32_t i=0;i<CNK_ITEM_MAX_BOMBS;i++){
  const cnk_item_bomb *p=&world->bombs[i];if(!p->active)continue;
  if(!isfinite(p->heading)||!isfinite(p->lifetime)||p->lifetime<=0.0f ||
     append(&tmp,p->position,0.6f,CNK_ITEM_SPRITE_BOMB))return -4;
 }
 for(uint32_t i=0;i<CNK_ITEM_MAX_MISSILES;i++){
  const cnk_item_missile *p=&world->missiles[i];if(!p->active)continue;
  if(!isfinite(p->heading)||!isfinite(p->lifetime)||p->lifetime<=0.0f ||
     append(&tmp,p->position,0.65f,CNK_ITEM_SPRITE_MISSILE))return -5;
 }
 for(uint32_t i=0;i<CNK_ITEM_MAX_HAZARDS;i++){
  const cnk_item_hazard *p=&world->hazards[i];if(!p->active)continue;
  if(!isfinite(p->lifetime)||p->lifetime<=0.0f ||
      (p->kind!=CNK_WEAPON_TNT&&p->kind!=CNK_WEAPON_OIL)||
     append(&tmp,p->position,p->kind==CNK_WEAPON_TNT?0.7f:0.55f,
        p->kind==CNK_WEAPON_TNT?CNK_ITEM_SPRITE_TNT:CNK_ITEM_SPRITE_OIL))return -6;
 }
 *out=tmp;return 0;
}
