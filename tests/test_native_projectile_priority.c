#include "cnk_items.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void case_projectile(int missile){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 /* Slot 2 is encountered first. Slot 1 is farther ahead. */
 k[1].position=(cnk_vec3){0,0,1.4f};
 k[2].position=(cnk_vec3){0,0,0.8f};
 before[0]=k[0].position;before[1]=k[1].position;before[2]=k[2].position;
 k[1].speed=9;k[2].speed=9;
 if(missile){
  w.missiles[0].active=1;w.missiles[0].owner=0;
  w.missiles[0].target=UINT32_MAX;
  w.missiles[0].position=(cnk_vec3){0,0,0};
  w.missiles[0].heading=0;w.missiles[0].lifetime=2.0f;
 }else{
  w.bombs[0].active=1;w.bombs[0].owner=0;
  w.bombs[0].position=(cnk_vec3){0,0,0};
  w.bombs[0].heading=0;w.bombs[0].lifetime=2.0f;
 }
 assert(!cnk_items_step(&w,k,before,movable,3,0.10f,events));
 assert(events[2]&CNK_ITEM_EVENT_HIT);
 assert(!(events[1]&CNK_ITEM_EVENT_HIT));
 assert(k[2].speed<9 && k[1].speed==9);
 assert(missile ? !w.missiles[0].active : !w.bombs[0].active);
}
static void case_height_at_collision(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 w.bombs[0].active=1;w.bombs[0].owner=0;
 w.bombs[0].position=(cnk_vec3){0,0,0};w.bombs[0].lifetime=2.0f;
 before[1]=(cnk_vec3){0,10,0.8f};
 k[1].position=(cnk_vec3){0,0,0.8f};k[1].speed=8;
 assert(!cnk_items_step(&w,k,before,movable,2,0.10f,events));
 assert(!(events[1]&CNK_ITEM_EVENT_HIT));
 assert(k[1].speed==8 && w.bombs[0].active);
}
int main(void){
 case_projectile(0);case_projectile(1);case_height_at_collision();
 puts("PASS: first-hit ordering for bomb/missile, contact-time Y, no duplicate hit");
 return 0;
}
