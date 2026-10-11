#include "cnk_items.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void case_hazard(cnk_weapon weapon){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 w.hazards[0].active=1;w.hazards[0].owner=0;
 w.hazards[0].position=(cnk_vec3){0,0,0};w.hazards[0].kind=weapon;
 w.hazards[0].lifetime=5;
 /* Slot 2 crosses the contact disc before slot 1 in the same tick. */
 before[1]=(cnk_vec3){-4,0,0};k[1].position=(cnk_vec3){4,0,0};
 before[2]=(cnk_vec3){-1.5f,0,0};k[2].position=(cnk_vec3){1,0,0};
 k[1].speed=k[2].speed=10;
 assert(!cnk_items_step(&w,k,before,movable,3,1.0f/60.0f,events));
 assert(!w.hazards[0].active);
 assert(events[2]&CNK_ITEM_EVENT_HIT);
 assert(!(events[1]&CNK_ITEM_EVENT_HIT));
 assert(k[1].speed==10 && k[2].speed<10);
 if(weapon==CNK_WEAPON_TNT)assert(events[2]&CNK_ITEM_EVENT_TNT_HIT);
 else assert(events[2]&CNK_ITEM_EVENT_OIL_HIT);
}
static void case_height(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t live[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 w.hazards[0].active=1;w.hazards[0].owner=0;w.hazards[0].kind=CNK_WEAPON_OIL;
 w.hazards[0].lifetime=5;
 before[1]=(cnk_vec3){-2,12,0};k[1].position=(cnk_vec3){2,0,0};
 assert(!cnk_items_step(&w,k,before,live,2,1.0f/60.0f,events));
 assert(!(events[1]&CNK_ITEM_EVENT_HIT));
 assert(w.hazards[0].active);
}
int main(void){
 case_hazard(CNK_WEAPON_TNT);case_hazard(CNK_WEAPON_OIL);case_height();
 puts("PASS: first swept TNT/oil racer and contact-time height");
 return 0;
}
