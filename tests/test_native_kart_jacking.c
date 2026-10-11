#include "cnk_items.h"
#include "cnk_lang.h"
#include "cnk_app_runtime.h"
#include "cnk_item_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void source_text(const char *path){
 fufs_file data={0};cnk_lang_pack p={0};
 assert(!fufs_open(path,&data));
 assert(!cnk_lang_load(&data,CNK_LANG_ENGLISH,&p));
 const char *text=cnk_lang_text(&p,135);
 assert(text&&strstr(text,"KART JACKING:")&&strstr(text,"SWITCHES PLACE"));
 cnk_lang_free(&p);fufs_close(&data);
}
static void swap_and_rejection(void){
 cnk_item_world w={0};cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 uint8_t eligible[CNK_ITEM_MAX_RACERS]={1,1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 k[0].position=(cnk_vec3){0,0,0};k[0].heading=0;k[0].speed=7;
 k[1].position=(cnk_vec3){0,0,12};k[1].heading=1;k[1].speed=9;
 k[2].position=(cnk_vec3){0,0,5};k[2].heading=2;k[2].speed=4;
 w.racers[0].held=CNK_WEAPON_KART_JACKING;
 /* The sector mask comes from validated lap/checkpoint progress. */
 eligible[2]=0;
 assert(cnk_items_try_jack(&w,0,k,eligible,3,events)==2);
 assert(w.racers[0].held==CNK_WEAPON_NONE);
 assert(k[0].position.z==12&&k[1].position.z==0);
 assert(k[0].heading==1&&k[1].heading==0);
 assert(k[0].speed==7&&k[1].speed==9);
 assert(k[2].position.z==5&&k[2].heading==2);
 assert(events[0]&CNK_ITEM_EVENT_KART_JACK);
 w.racers[0].held=CNK_WEAPON_KART_JACKING;
 eligible[1]=0;
 cnk_item_world previous=w;
 cnk_kart old_karts[CNK_ITEM_MAX_RACERS];
 memcpy(old_karts,k,sizeof(k));
 uint32_t prior=events[0];
 assert(cnk_items_try_jack(&w,0,k,eligible,3,events)==0);
 assert(!memcmp(&previous,&w,sizeof(w)));
 assert(!memcmp(k,old_karts,sizeof(k))&&events[0]==prior);
 eligible[1]=1;
 /* Solo practice mode cannot abort the session merely because no rival exists. */
 assert(cnk_items_try_jack(&w,0,k,eligible,1,events)==0);
 assert(w.racers[0].held==CNK_WEAPON_KART_JACKING);
 k[1].position.x=NAN;
 assert(cnk_items_try_jack(&w,0,k,eligible,3,events)<0);
 assert(w.racers[0].held==CNK_WEAPON_KART_JACKING);
 assert(events[0]==prior);
}
static void rom_test(const char *path){
 fufs_file data={0};cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!fufs_open(path,&data));
 assert(!cnk_app_runtime_init(&app,&data,2,2));
 uint64_t us=1000000;
 assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&snap));
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_CONFIRM,(cnk_driver_input){0},&snap));
 assert(snap.state==CNK_FRONTEND_RACE);
 cnk_session *s=&app.frontend.game.session;
 s->items.racers[0].held=CNK_WEAPON_KART_JACKING;
 us+=16667;assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&snap));
 assert(s->items.racers[0].held==CNK_WEAPON_KART_JACKING);
 for(int i=0;i<232;i++){
  us+=16667;assert(!cnk_app_runtime_tick(&app,us,0,(cnk_driver_input){0},&snap));
 }
 /* Establish one verified sector; artificially pose the opponent nearby
  * inside this SAME sector to exercise safe interception, not invented ROM
  * placements or original AI. */
 assert(s->racers[0].race.completed_laps==s->racers[1].race.completed_laps);
 s->racers[1].race.next_gate=s->racers[0].race.next_gate;
 cnk_kart player=s->racers[0].kart;
 s->racers[1].kart.position.x=player.position.x+sinf(player.heading)*5.0f;
 s->racers[1].kart.position.y=player.position.y;
 s->racers[1].kart.position.z=player.position.z+cosf(player.heading)*5.0f;
 s->racers[1].kart.heading=player.heading;
 cnk_vec3 nearby=s->racers[1].kart.position;
 uint32_t start_laps=s->racers[0].race.completed_laps;
 uint32_t start_gate=s->racers[0].race.next_gate;
 us+=16667;
 assert(!cnk_app_runtime_tick(&app,us,CNK_UI_ITEM,(cnk_driver_input){0},&snap));
 assert(snap.item_events[0]&CNK_ITEM_EVENT_KART_JACK);
 assert(s->items.racers[0].held==CNK_WEAPON_NONE);
 assert(fabsf(s->racers[0].kart.position.x-nearby.x)<1.5f);
 assert(fabsf(s->racers[0].kart.position.z-nearby.z)<1.5f);
 assert(s->racers[0].race.completed_laps==start_laps);
 assert(s->racers[0].race.next_gate==start_gate);
 assert(!(s->racers[0].last_event&(CNK_RACE_LAP|CNK_RACE_FINISH)));
 cnk_app_runtime_close(&app);fufs_close(&data);
}
int main(int argc,char **argv){
 assert(argc==2);source_text(argv[1]);swap_and_rejection();rom_test(argv[1]);
 puts("PASS: original Kart Jacking, sector eligibility, atomic swap, ROM race without free laps");
 return 0;
}
