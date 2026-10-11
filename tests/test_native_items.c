#include "cnk_items.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static void run(cnk_item_world *w,cnk_kart k[],cnk_vec3 before[],
                uint8_t movable[],int count,float dt,uint32_t events[]){
 assert(!cnk_items_step(w,k,before,movable,(uint32_t)count,dt,events));
}
int main(void){
 cnk_item_world world={0};
 cnk_vec3 pos={0,0,0};
 assert(cnk_items_place(NULL,CNK_PICKUP_WUMPA,pos,1,CNK_WEAPON_NONE)<0);
 assert(cnk_items_place(&world,CNK_PICKUP_WUMPA,pos,NAN,CNK_WEAPON_NONE)<0);
 assert(cnk_items_place(&world,CNK_PICKUP_CRATE,pos,1,CNK_WEAPON_NONE)<0);
 assert(cnk_items_place(&world,CNK_PICKUP_WUMPA,pos,1,CNK_WEAPON_TURBO)<0);
 assert(world.pickup_count==0);
 assert(!cnk_items_place(&world,CNK_PICKUP_WUMPA,pos,1,CNK_WEAPON_NONE));
 assert(!cnk_items_place(&world,CNK_PICKUP_CRATE,pos,1,CNK_WEAPON_BOMB));
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t valid[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 k[0].position=(cnk_vec3){-2,0,0};k[1].position=(cnk_vec3){2,0,0};
 before[0]=k[0].position;before[1]=k[1].position;
 k[0].position=(cnk_vec3){2,0,0};k[1].position=(cnk_vec3){-2,0,0};
 /* Both swept through the same two pickups. Stable ownership, no double-award. */
 run(&world,k,before,valid,2,1.0f/60,events);
 assert(world.racers[0].wumpa==1 && world.racers[1].wumpa==0);
 assert(world.racers[0].held==CNK_WEAPON_BOMB && world.racers[1].held==CNK_WEAPON_NONE);
 assert((events[0]&(CNK_ITEM_EVENT_WEAPON|CNK_ITEM_EVENT_WUMPA))==
        (CNK_ITEM_EVENT_WEAPON|CNK_ITEM_EVENT_WUMPA));
 assert(events[1]==0);
 events[0]=events[1]=0;
 run(&world,k,before,valid,2,1.0f/60,events);
 assert(events[0]==0 && events[1]==0); /* cooldown */
 uint32_t used=0;
 assert(cnk_items_use(&world,0,&k[0],&used)==1);
 assert((used&(CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_BOMB_FIRED))==
        (CNK_ITEM_EVENT_USED|CNK_ITEM_EVENT_BOMB_FIRED));
 assert(world.racers[0].held==CNK_WEAPON_NONE);
 int live=0;for(int i=0;i<CNK_ITEM_MAX_BOMBS;i++)live+=world.bombs[i].active;
 assert(live==1);
 /* Bomb flies from kart 0 towards kart 1 and applies a tangible slowdown. */
 memset(&world.bombs,0,sizeof(world.bombs));
 cnk_kart shooter={0};shooter.position=(cnk_vec3){0,0,0};shooter.heading=0;
 world.racers[0].held=CNK_WEAPON_BOMB;
 assert(cnk_items_use(&world,0,&shooter,&used)==1);
 k[0]=shooter;k[1].position=(cnk_vec3){0,0,2.0f};k[1].speed=8;
 for(int i=0;i<10 && !(events[1]&CNK_ITEM_EVENT_HIT);i++){
  before[0]=k[0].position;before[1]=k[1].position;
  run(&world,k,before,valid,2,1.0f/60,events);
 }
 assert(events[1]&CNK_ITEM_EVENT_HIT);
 assert(k[1].speed<8 && world.racers[1].stun_seconds>0);
 assert(!world.bombs[0].active);
 /* A second explosion cannot damage the same kart. */
 float speed=k[1].speed;
 before[1]=k[1].position;run(&world,k,before,valid,2,1.0f/60,events);
 assert(k[1].speed==speed);
 /* Use turbo item, apply boosted real tuning, then finish cleanly. */
 world.racers[0].held=CNK_WEAPON_TURBO;
 used=0;
 assert(cnk_items_use(&world,0,&shooter,&used)==1);
 assert(used&(CNK_ITEM_EVENT_TURBO_ON|CNK_ITEM_EVENT_USED));
 cnk_drive_tuning base={0},faster={0};cnk_drive_default_tuning(&base);
 assert(!cnk_items_tune(&world,0,&base,&faster));
 assert(faster.max_speed>base.max_speed && faster.acceleration>base.acceleration);
 for(int i=0;i<90;i++){
  before[0]=k[0].position;before[1]=k[1].position;
  run(&world,k,before,valid,2,1.0f/60,events);
 }
 assert(world.racers[0].turbo_seconds==0);
 assert(events[0]&CNK_ITEM_EVENT_TURBO_OFF);
 assert(!cnk_items_tune(&world,0,&base,&faster));
 assert(faster.max_speed==base.max_speed);
 world.pickup_count=0;world.racers[0].wumpa=10;
 assert(!cnk_items_place(&world,CNK_PICKUP_WUMPA,shooter.position,1,CNK_WEAPON_NONE));
 before[0]=k[0].position;before[1]=k[1].position;
 run(&world,k,before,valid,2,1.0f/60,events);
 assert(world.racers[0].wumpa==10); /* capped */
 /* A full projectile pool refuses fire without losing the held item. */
 for(int i=0;i<CNK_ITEM_MAX_BOMBS;i++)world.bombs[i].active=1;
 world.racers[0].held=CNK_WEAPON_BOMB;
 used=0;
 assert(cnk_items_use(&world,0,&shooter,&used)==0);
 assert(world.racers[0].held==CNK_WEAPON_BOMB && !used);
 /* Provisional NPC behavior uses the same projectile pool and hit math. */
 cnk_item_world npc={0};uint32_t ai_event=0;
 cnk_kart bots[CNK_ITEM_MAX_RACERS]={0};uint8_t move[CNK_ITEM_MAX_RACERS]={1,1};
 bots[1].position=(cnk_vec3){0,0,0};bots[1].heading=0;bots[1].speed=6;
 bots[0].position=(cnk_vec3){10,0,0};
 npc.racers[1].held=CNK_WEAPON_BOMB;
 assert(!cnk_items_ai_use(&npc,1,bots,move,2,&ai_event));
 assert(npc.racers[1].held==CNK_WEAPON_BOMB);
 bots[0].position=(cnk_vec3){0,0,3.0f};
 assert(cnk_items_ai_use(&npc,1,bots,move,2,&ai_event)==1);
 assert(npc.racers[1].held==CNK_WEAPON_NONE);
 assert(ai_event&CNK_ITEM_EVENT_BOMB_FIRED);
 npc.racers[1].held=CNK_WEAPON_TURBO;bots[1].speed=0;
 assert(!cnk_items_ai_use(&npc,1,bots,move,2,&ai_event));
 assert(npc.racers[1].held==CNK_WEAPON_TURBO);
 bots[1].speed=7;
 assert(cnk_items_ai_use(&npc,1,bots,move,2,&ai_event)==1);
 assert(npc.racers[1].turbo_seconds>0);
 puts("PASS: pickup swept collisions, inventory, Wumpa cap, cooldown, bomb impact, item turbo, no item loss");
}
