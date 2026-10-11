#include "cnk_items.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void expect_unchanged(cnk_item_world *w,cnk_kart *k,
                             cnk_vec3 *before,uint8_t *live,uint32_t *events){
 cnk_item_world saved_world=*w;
 cnk_kart saved_karts[CNK_ITEM_MAX_RACERS];
 uint32_t saved_events[CNK_ITEM_MAX_RACERS];
 memcpy(saved_karts,k,sizeof(saved_karts));
 memcpy(saved_events,events,sizeof(saved_events));
 assert(cnk_items_step(w,k,before,live,2,.016f,events)<0);
 assert(!memcmp(&saved_world,w,sizeof(saved_world)));
 assert(!memcmp(saved_karts,k,sizeof(saved_karts)));
 assert(!memcmp(saved_events,events,sizeof(saved_events)));
}
int main(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t live[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={1u,2u};
 before[0]=(cnk_vec3){0,0,0};k[0].position=(cnk_vec3){10,0,0};
 before[1]=(cnk_vec3){0,0,0};k[1].position=(cnk_vec3){10,0,0};
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){2,0,0},.5f,CNK_WEAPON_NONE));
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){8,0,0},.5f,CNK_WEAPON_NONE));
 /* Malformed late pickup after an otherwise valid collection. */
 w.pickups[1].kind=CNK_PICKUP_CRATE;
 w.pickups[1].reward=CNK_WEAPON_NONE;
 expect_unchanged(&w,k,before,live,events);
 w.pickups[1].kind=CNK_PICKUP_WUMPA;
 /* Missing finiteness in any active projectile must not consume Wumpa. */
 w.bombs[0].active=1;w.bombs[0].owner=0;
 w.bombs[0].heading=NAN;
 expect_unchanged(&w,k,before,live,events);
 w.bombs[0].active=0;
 /* Invalid initial racer coordinates must not alter the inventory. */
 k[1].heading=INFINITY;
 expect_unchanged(&w,k,before,live,events);
 k[1].heading=0;
 assert(!cnk_items_step(&w,k,before,live,2,.016f,events));
 assert(w.racers[0].wumpa+w.racers[1].wumpa==2);
 assert(w.pickups[0].unavailable_seconds>0&&w.pickups[1].unavailable_seconds>0);
 puts("PASS: ordinary item tick commits only on success, preserving objects and events on failure");
 return 0;
}
