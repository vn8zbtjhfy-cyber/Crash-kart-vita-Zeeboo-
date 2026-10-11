#include "cnk_items.h"
#include "cnk_lang.h"
#include "cnk_item_layout.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static void source_proof(const char *path){
 fufs_file data={0};cnk_lang_pack lang={0};
 assert(!fufs_open(path,&data));
 assert(!cnk_lang_load(&data,CNK_LANG_ENGLISH,&lang));
 const char *description=cnk_lang_text(&lang,135);
 assert(description && strstr(description,"AKU-AKU:") &&
        strstr(description,"BOOST OF SPEED") && strstr(description,"TACKLES"));
 cnk_lang_free(&lang);fufs_close(&data);
}
static void activation_and_tuning(void){
 cnk_item_world world={0};cnk_kart kart={0};uint32_t event=0;
 cnk_drive_tuning standard={0},actual={0};
 cnk_drive_default_tuning(&standard);
 world.racers[0].held=CNK_WEAPON_AKU_AKU;
 assert(cnk_items_use(&world,0,&kart,&event)==1);
 assert(world.racers[0].held==CNK_WEAPON_NONE);
 assert(world.racers[0].aku_seconds>0);
 assert((event&(CNK_ITEM_EVENT_AKU_ON|CNK_ITEM_EVENT_USED))==
        (CNK_ITEM_EVENT_AKU_ON|CNK_ITEM_EVENT_USED));
 assert(!cnk_items_tune(&world,0,&standard,&actual));
 assert(actual.max_speed>standard.max_speed);
 assert(actual.acceleration>standard.acceleration);
 cnk_kart karts[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 for(int i=0;i<181;i++){
  assert(!cnk_items_step(&world,karts,before,movable,1,1.0f/60.0f,events));
 }
 assert(world.racers[0].aku_seconds==0);
 assert(events[0]&CNK_ITEM_EVENT_AKU_OFF);
 assert(!cnk_items_tune(&world,0,&standard,&actual));
 assert(actual.max_speed==standard.max_speed);
}
static void tackle(int shield){
 cnk_item_world w={0};cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 k[0].position=(cnk_vec3){0,0,2}; k[0].speed=8; k[0].heading=0;
 k[1].position=(cnk_vec3){0,0,1}; k[1].speed=8;
 before[0]=(cnk_vec3){0,0,0};before[1]=k[1].position;
 w.racers[0].held=CNK_WEAPON_AKU_AKU;
 assert(cnk_items_use(&w,0,&k[0],&events[0])==1);
 w.racers[1].wumpa=10;
 if(shield)w.racers[1].shield_seconds=2;
 assert(!cnk_items_step(&w,k,before,movable,2,1.0f/60.0f,events));
 assert(events[0]&CNK_ITEM_EVENT_AKU_TACKLE);
 if(shield){
  assert(k[1].speed==8 && w.racers[1].wumpa==10);
  assert(events[1]&CNK_ITEM_EVENT_SHIELD_BLOCK);
 }else{
  assert(k[1].speed<8 && w.racers[1].wumpa==8);
  assert(events[1]&CNK_ITEM_EVENT_HIT);
 }
 /* Even an overlapping racer cannot be tackled every frame. */
 uint32_t event2[CNK_ITEM_MAX_RACERS]={0};
 assert(!cnk_items_step(&w,k,before,movable,2,1.0f/60.0f,event2));
 assert(!(event2[0]&CNK_ITEM_EVENT_AKU_TACKLE));
 assert(w.racers[0].aku_tackle_cooldown>0);
}
static void atomic_failure_and_placement(void){
 cnk_item_world w={0};cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 w.racers[0].aku_seconds=NAN;
 cnk_item_world unchanged=w;
 assert(cnk_items_step(&w,k,before,movable,1,1.0f/60.0f,events)<0);
 assert(memcmp(&w,&unchanged,sizeof(w))==0);
 assert(events[0]==0);
 assert(cnk_items_use(&w,0,&k[0],&events[0])==0);
 w.racers[0].aku_seconds=0;
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){0,0,0},1,
                          CNK_WEAPON_AKU_AKU));
 assert(!cnk_items_step(&w,k,before,movable,1,1.0f/60.0f,events));
 assert(w.racers[0].held==CNK_WEAPON_AKU_AKU);
}
static void rom_race(const char *path){
 fufs_file data={0};cnk_app_runtime app={0};cnk_app_snapshot sn={0};
 assert(!fufs_open(path,&data));
 assert(!cnk_app_runtime_init(&app,&data,2,2));
 uint64_t us=1000000;
 assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_CONFIRM,(cnk_driver_input){0},&sn));
 assert(sn.state==CNK_FRONTEND_RACE);
 app.frontend.game.session.items.racers[0].held=CNK_WEAPON_AKU_AKU;
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(app.frontend.game.session.items.racers[0].held==CNK_WEAPON_AKU_AKU);
 for(int i=0;i<232;i++){
  us+=16667;assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 }
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(sn.player_items.aku_seconds>0);
 assert(sn.item_events[0]&CNK_ITEM_EVENT_AKU_ON);
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_PAUSE,(cnk_driver_input){0},&sn));
 float frozen=app.frontend.game.session.items.racers[0].aku_seconds;
 for(int i=0;i<15;i++){
  us+=16667;assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 }
 assert(app.frontend.game.session.items.racers[0].aku_seconds==frozen);
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_RESTART,(cnk_driver_input){0},&sn));
 assert(app.frontend.game.session.items.racers[0].aku_seconds==0);
 cnk_app_runtime_close(&app);fufs_close(&data);
}
int main(int argc,char **argv){
 assert(argc==2);source_proof(argv[1]);activation_and_tuning();
 tackle(0);tackle(1);atomic_failure_and_placement();rom_race(argv[1]);
 puts("PASS: original Aku-Aku instructions, boost, tackle, shield, cooldown, atomic checks, ROM application");
 return 0;
}
