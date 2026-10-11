#include "cnk_items.h"
#include "cnk_lang.h"
#include "cnk_item_layout.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* Zeebo LANG explicitly describes SHIELD and the 10-Wumpa speed benefit.
 * We test the NEW provisional mechanics against a ROM-backed description,
 * without claiming the numeric effects are original gameplay tuning. */
static void original_description(const char *path){
 fufs_file f={0};cnk_lang_pack pack={0};
 assert(!fufs_open(path,&f));
 assert(!cnk_lang_load(&f,CNK_LANG_ENGLISH,&pack));
 const char *help=cnk_lang_text(&pack,135);
 assert(help&&strstr(help,"SHIELD:")&&strstr(help,"WUMPA FRUIT:"));
 assert(strstr(help,"PROTECTS YOU FROM ALL HARM"));
 assert(strstr(help,"COLLECT AND KEEP 10"));
 cnk_lang_free(&pack);fufs_close(&f);
}
static void active_impact(cnk_weapon weapon,int shield){
 cnk_item_world w={0};cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 k[0].position=(cnk_vec3){0,0,0};k[0].heading=0;
 k[1].position=(cnk_vec3){0,0,weapon==CNK_WEAPON_TNT||weapon==CNK_WEAPON_OIL?-1.3f:1.4f};
 k[1].heading=0;k[1].speed=10.0f;
 before[0]=k[0].position;before[1]=k[1].position;
 w.racers[1].wumpa=10;
 if(shield){
  w.racers[1].held=CNK_WEAPON_SHIELD;
  assert(cnk_items_use(&w,1,&k[1],&events[1])==1);
  assert(w.racers[1].held==CNK_WEAPON_NONE && w.racers[1].shield_seconds>0);
  assert((events[1]&(CNK_ITEM_EVENT_SHIELD_ON|CNK_ITEM_EVENT_USED))==
         (CNK_ITEM_EVENT_SHIELD_ON|CNK_ITEM_EVENT_USED));
 }
 w.racers[0].held=weapon;
 assert(cnk_items_use(&w,0,&k[0],&events[0])==1);
 assert(!cnk_items_step(&w,k,before,movable,2,1.0f/60.0f,events));
 if(shield){
  assert(k[1].speed==10.0f);
  assert(w.racers[1].wumpa==10u);
  assert(w.racers[1].stun_seconds==0 && w.racers[1].oil_seconds==0);
  assert(events[1]&CNK_ITEM_EVENT_SHIELD_BLOCK);
  assert(!(events[1]&(CNK_ITEM_EVENT_HIT|CNK_ITEM_EVENT_WUMPA_LOST)));
 }else{
  assert(k[1].speed<10.0f);
  assert(w.racers[1].wumpa==8u);
  assert(events[1]&CNK_ITEM_EVENT_HIT);
  assert(events[1]&CNK_ITEM_EVENT_WUMPA_LOST);
 }
 if(weapon==CNK_WEAPON_MISSILE)assert(!w.missiles[0].active);
 if(weapon==CNK_WEAPON_BOMB)assert(!w.bombs[0].active);
 if(weapon==CNK_WEAPON_TNT||weapon==CNK_WEAPON_OIL)assert(!w.hazards[0].active);
}
static void wumpa_speed_and_shield_lifetime(void){
 cnk_item_world w={0};cnk_drive_tuning base={0},normal={0},boost={0};
 cnk_drive_default_tuning(&base);
 w.racers[0].wumpa=9;
 assert(!cnk_items_tune(&w,0,&base,&normal));
 assert(normal.max_speed==base.max_speed);
 w.racers[0].wumpa=10;
 assert(!cnk_items_tune(&w,0,&base,&boost));
 assert(boost.max_speed>normal.max_speed);
 assert(boost.acceleration>normal.acceleration);
 w.racers[0].wumpa=8;
 assert(!cnk_items_tune(&w,0,&base,&boost));
 assert(boost.max_speed==normal.max_speed);
 /* Reject corrupt item state atomically. */
 w.racers[0].wumpa=11;boost.max_speed=12345;
 assert(cnk_items_tune(&w,0,&base,&boost)<0);
 assert(boost.max_speed==12345);
 w.racers[0].wumpa=0;
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 w.racers[0].held=CNK_WEAPON_SHIELD;
 assert(cnk_items_use(&w,0,&k[0],&events[0])==1);
 for(int i=0;i<152;i++){
  before[0]=k[0].position;
  assert(!cnk_items_step(&w,k,before,movable,1,1.0f/60.0f,events));
 }
 assert(w.racers[0].shield_seconds==0);
 assert(events[0]&CNK_ITEM_EVENT_SHIELD_OFF);
}
static void reviewed_layout_shield(void){
 char name[]="/tmp/cnk90_shield_XXXXXX";
 int fd=mkstemp(name);assert(fd>=0);
 FILE *fp=fdopen(fd,"w");assert(fp);
 assert(fputs("12345\tCRATE\t0\t0\t0\t1\tSHIELD\n",fp)>=0);
 assert(!fclose(fp));
 cnk_item_layout l={0};
 assert(!cnk_item_layout_read(name,&l));
 assert(l.count==1 && l.entries[0].pickup.reward==CNK_WEAPON_SHIELD);
 remove(name);
}
/* One native simulation loaded from the ROM, including the countdown gate,
 * L item request, pause and restart. Uses no Vita emulator or VPK. */
static void original_track_shield(const char *path){
 fufs_file vfs={0};cnk_app_runtime app={0};cnk_app_snapshot sn={0};
 assert(!fufs_open(path,&vfs));
 assert(!cnk_app_runtime_init(&app,&vfs,2,2));
 uint64_t us=1000000u;
 assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 us+=16667u;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_CONFIRM,(cnk_driver_input){0},&sn));
 assert(sn.state==CNK_FRONTEND_RACE);
 app.frontend.game.session.items.racers[0].held=CNK_WEAPON_SHIELD;
 us+=16667u;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(app.frontend.game.session.items.racers[0].held==CNK_WEAPON_SHIELD);
 for(int i=0;i<231;i++){
  us+=16667u;
  assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 }
 assert(sn.start_phase==CNK_START_GO||sn.start_phase==CNK_START_RUNNING);
 us+=16667u;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(sn.item_events[0]&CNK_ITEM_EVENT_SHIELD_ON);
 assert(sn.player_items.shield_seconds>0);
 us+=16667u;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_PAUSE,(cnk_driver_input){0},&sn));
 float frozen=app.frontend.game.session.items.racers[0].shield_seconds;
 for(int i=0;i<20;i++){
  us+=16667u;
  assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&sn));
 }
 assert(app.frontend.game.session.items.racers[0].shield_seconds==frozen);
 us+=16667u;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_RESTART,(cnk_driver_input){0},&sn));
 assert(sn.start_phase==CNK_START_THREE);
 assert(sn.player_items.shield_seconds==0);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
}

int main(int argc,char **argv){
 assert(argc==2);original_description(argv[1]);
 for(int k=CNK_WEAPON_BOMB;k<=CNK_WEAPON_OIL;k++){
  if(k==CNK_WEAPON_TNT||k==CNK_WEAPON_OIL||k==CNK_WEAPON_MISSILE||k==CNK_WEAPON_BOMB){
   active_impact((cnk_weapon)k,0);active_impact((cnk_weapon)k,1);
  }
 }
 wumpa_speed_and_shield_lifetime();reviewed_layout_shield();
 original_track_shield(argv[1]);
 puts("PASS: source LANG provenance, 10 Wumpa speed, SHIELD blocks all four attack types, Wumpa loss on real hit, lifetime and reviewed layout");
 return 0;
}
