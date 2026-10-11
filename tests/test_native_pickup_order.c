#include "cnk_items.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_crates_arrival_not_index(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 /* Later crate is intentionally stored first. */
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){8,0,0},.5f,CNK_WEAPON_MISSILE));
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){2,0,0},.5f,CNK_WEAPON_TNT));
 before[0]=(cnk_vec3){0,0,0};k[0].position=(cnk_vec3){10,0,0};
 assert(!cnk_items_step(&w,k,before,movable,1,.016f,events));
 assert(w.racers[0].held==CNK_WEAPON_TNT);
 assert(w.pickups[1].unavailable_seconds>0);
 assert(w.pickups[0].unavailable_seconds==0);
 assert((events[0]&CNK_ITEM_EVENT_WEAPON)!=0);
}
static void test_reached_crate_goes_to_eligible_second_racer(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){8,0,0},.5f,CNK_WEAPON_MISSILE));
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){2,0,0},.5f,CNK_WEAPON_TNT));
 before[0]=(cnk_vec3){0,0,0};k[0].position=(cnk_vec3){10,0,0};
 before[1]=(cnk_vec3){5,0,0};k[1].position=(cnk_vec3){10,0,0};
 assert(!cnk_items_step(&w,k,before,movable,2,.016f,events));
 assert(w.racers[0].held==CNK_WEAPON_TNT);
 assert(w.racers[1].held==CNK_WEAPON_MISSILE);
 assert(w.pickups[0].unavailable_seconds>0 && w.pickups[1].unavailable_seconds>0);
}
static void test_wumpa_chain_before_crate_and_height(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){8,0,0},.5f,CNK_WEAPON_OIL));
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){4,0,0},.5f,CNK_WEAPON_NONE));
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){2,0,0},.5f,CNK_WEAPON_NONE));
 before[0]=(cnk_vec3){0,0,0};k[0].position=(cnk_vec3){10,0,0};
 assert(!cnk_items_step(&w,k,before,movable,1,.016f,events));
 assert(w.racers[0].wumpa==2 && w.racers[0].held==CNK_WEAPON_OIL);
 assert((events[0]&(CNK_ITEM_EVENT_WUMPA|CNK_ITEM_EVENT_WEAPON))==
                   (CNK_ITEM_EVENT_WUMPA|CNK_ITEM_EVENT_WEAPON));
}
int main(void){
 test_crates_arrival_not_index();
 test_reached_crate_goes_to_eligible_second_racer();
 test_wumpa_chain_before_crate_and_height();
 puts("PASS: same-frame pickups sorted by physical arrival not placement index");
 return 0;
}
