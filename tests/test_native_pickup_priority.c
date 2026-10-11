#include "cnk_items.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_first_arrival(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 b[CNK_ITEM_MAX_RACERS]={0};
 uint8_t live[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t ev[CNK_ITEM_MAX_RACERS]={0};
 assert(cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){0,0,0},0.5f,CNK_WEAPON_BOMB)==0);
 b[0]=(cnk_vec3){-4,0,0};k[0].position=(cnk_vec3){4,0,0};
 b[1]=(cnk_vec3){-1.5f,0,0};k[1].position=(cnk_vec3){1,0,0};
 assert(!cnk_items_step(&w,k,b,live,2,1.0f/60.0f,ev));
 assert(w.racers[0].held==CNK_WEAPON_NONE);
 assert(w.racers[1].held==CNK_WEAPON_BOMB);
 assert(ev[0]==0&&(ev[1]&CNK_ITEM_EVENT_WEAPON));
}
static void test_tie_and_ineligible(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 b[CNK_ITEM_MAX_RACERS]={0};
 uint8_t live[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t ev[CNK_ITEM_MAX_RACERS]={0};
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){0,0,0},0.5f,CNK_WEAPON_MISSILE));
 b[0]=b[1]=(cnk_vec3){-2,0,0};
 k[0].position=k[1].position=(cnk_vec3){2,0,0};
 assert(!cnk_items_step(&w,k,b,live,2,0.016f,ev));
 assert(w.racers[0].held==CNK_WEAPON_MISSILE && w.racers[1].held==CNK_WEAPON_NONE);
 w.pickups[0].unavailable_seconds=0;
 w.racers[1].held=CNK_WEAPON_NONE;
 memset(ev,0,sizeof(ev));
 assert(!cnk_items_step(&w,k,b,live,2,0.016f,ev));
 assert(w.racers[1].held==CNK_WEAPON_MISSILE && !(ev[0]&CNK_ITEM_EVENT_WEAPON));
}
static void test_height_at_entry(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 b[CNK_ITEM_MAX_RACERS]={0};
 uint8_t live[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t ev[CNK_ITEM_MAX_RACERS]={0};
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){0,0,0},0.5f,CNK_WEAPON_NONE));
 b[0]=(cnk_vec3){-2,9,0};k[0].position=(cnk_vec3){2,0,0};
 b[1]=(cnk_vec3){-2,0,0};k[1].position=(cnk_vec3){2,0,0};
 assert(!cnk_items_step(&w,k,b,live,2,0.016f,ev));
 assert(w.racers[0].wumpa==0&&w.racers[1].wumpa==1);
}
int main(void){
 test_first_arrival();test_tie_and_ineligible();test_height_at_entry();
 puts("PASS: earliest swept pickup, deterministic tie, full inventory, contact-time height");
 return 0;
}
