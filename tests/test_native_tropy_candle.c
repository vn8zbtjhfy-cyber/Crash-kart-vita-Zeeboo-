/* Advancement 91: Zeebo LANG documents CLOCK and CURSED CANDLE.
 * The behavior here is reconstructed/provisional, NOT byte-identical Zeebo. */
#include "cnk_items.h"
#include "cnk_item_layout.h"
#include "cnk_app_runtime.h"
#include "cnk_lang.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void original_lang_proof(const char *path){
 fufs_file source={0};cnk_lang_pack en={0},es={0};
 assert(!fufs_open(path,&source));
 assert(!cnk_lang_load(&source,CNK_LANG_ENGLISH,&en));
 assert(!cnk_lang_load(&source,CNK_LANG_SPANISH,&es));
 assert(strstr(cnk_lang_text(&en,135),"TROPY CLOCK:"));
 assert(strstr(cnk_lang_text(&en,135),"SLOWS ALL OPPONENTS"));
 assert(strstr(cnk_lang_text(&en,135),"CURSED CANDLE:"));
 assert(strstr(cnk_lang_text(&en,135),"PUTS OUT THE LIGHTS"));
 assert(strstr(cnk_lang_text(&es,135),"VELA MALDITA"));
 cnk_lang_free(&en);cnk_lang_free(&es);fufs_close(&source);
}
static void source_reviewed_sidecar(void){
 char fname[]="/tmp/cnk91_items_XXXXXX";
 int fd=mkstemp(fname);assert(fd>=0);
 FILE *out=fdopen(fd,"w");assert(out);
 assert(fputs("1001\tCRATE\t0\t0\t0\t1\tTROPY_CLOCK\n"
              "1001\tCRATE\t2\t0\t0\t1\tCURSED_CANDLE\n",out)>=0);
 assert(!fclose(out));
 cnk_item_layout layout={0};
 assert(!cnk_item_layout_read(fname,&layout));
 assert(layout.count==2);
 assert(layout.entries[0].pickup.reward==CNK_WEAPON_TROPY_CLOCK);
 assert(layout.entries[1].pickup.reward==CNK_WEAPON_CURSED_CANDLE);
 assert(!remove(fname));
}
static void time_and_effects(void){
 cnk_item_world w={0};
 cnk_kart k[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t movable[CNK_ITEM_MAX_RACERS]={1,1,1};
 uint32_t events[CNK_ITEM_MAX_RACERS]={0};
 cnk_drive_tuning base={0},normal={0},slow={0};
 cnk_drive_default_tuning(&base);
 w.racers[0].held=CNK_WEAPON_TROPY_CLOCK;
 assert(cnk_items_use(&w,0,k,&events[0])==1);
 assert(w.racers[0].held==CNK_WEAPON_NONE);
 assert(w.clock_owner==0 && w.clock_seconds>0);
 assert(events[0]&CNK_ITEM_EVENT_CLOCK_ON);
 assert(!cnk_items_tune(&w,0,&base,&normal));
 assert(!cnk_items_tune(&w,1,&base,&slow));
 assert(normal.max_speed==base.max_speed);
 assert(slow.max_speed<normal.max_speed);
 assert(slow.acceleration<normal.acceleration);
 /* The clock does not mutate source tuning, only the chosen racer copy. */
 cnk_kart unrestricted={0},restricted={0};
 cnk_driver_input gas={1.0f,0.0f,0.0f};
 for(int i=0;i<60;i++){
  assert(!cnk_drive_step(&unrestricted,&normal,gas,1.0f/60.0f,NULL));
  assert(!cnk_drive_step(&restricted,&slow,gas,1.0f/60.0f,NULL));
 }
 assert(unrestricted.speed>restricted.speed);
 /* Other racer cannot steal the clock, and must keep the item. */
 w.racers[1].held=CNK_WEAPON_TROPY_CLOCK;
 assert(cnk_items_use(&w,1,&k[1],&events[1])==0);
 assert(w.racers[1].held==CNK_WEAPON_TROPY_CLOCK);
 /* Another independent candle can be activated while the clock runs. */
 w.racers[2].held=CNK_WEAPON_CURSED_CANDLE;
 assert(cnk_items_use(&w,2,&k[2],&events[2])==1);
 assert(w.candle_owner==2 && w.candle_seconds>0);
 assert(events[2]&CNK_ITEM_EVENT_CANDLE_ON);
 w.racers[0].held=CNK_WEAPON_CURSED_CANDLE;
 assert(cnk_items_use(&w,0,k,&events[0])==0);
 assert(w.racers[0].held==CNK_WEAPON_CURSED_CANDLE);
 /* Valid world is updated by transactional fixed 60Hz steps. */
 for(int i=0;i<242;i++){
  for(int j=0;j<3;j++)before[j]=k[j].position;
  assert(!cnk_items_step(&w,k,before,movable,3,1.0f/60.0f,events));
 }
 assert(w.clock_seconds==0.0f);
 assert(w.candle_seconds==0.0f);
 assert(events[0]&CNK_ITEM_EVENT_CLOCK_OFF);
 assert(events[2]&CNK_ITEM_EVENT_CANDLE_OFF);
 assert(!cnk_items_tune(&w,1,&base,&slow));
 assert(slow.max_speed==base.max_speed);
 assert(cnk_items_use(&w,1,&k[1],&events[1])==1);
 assert(w.clock_owner==1 && w.clock_seconds>0);
 /* Malformed timer or owner must not consume inventory or write output. */
 cnk_item_world corrupt=w;
 corrupt.candle_seconds=NAN;
 cnk_drive_tuning untouched=base;
 untouched.max_speed=432.0f;
 assert(cnk_items_tune(&corrupt,0,&base,&untouched)<0);
 assert(untouched.max_speed==432.0f);
 cnk_item_world snapshot=corrupt;
 uint32_t previous[CNK_ITEM_MAX_RACERS]={0};
 cnk_kart previous_karts[CNK_ITEM_MAX_RACERS]={0};
 memcpy(previous_karts,k,sizeof(k));
 assert(cnk_items_step(&corrupt,k,before,movable,3,1.0f/60.0f,previous)<0);
 assert(memcmp(&corrupt,&snapshot,sizeof(corrupt))==0);
 assert(memcmp(k,previous_karts,sizeof(k))==0);
 corrupt=w;
 corrupt.clock_owner=CNK_ITEM_MAX_RACERS;
 assert(cnk_items_step(&corrupt,k,before,movable,3,1.0f/60.0f,previous)<0);
}
static void native_road_clock_and_candle(const char *rom){
 fufs_file data={0};cnk_app_runtime app={0};cnk_app_snapshot sn={0};
 assert(!fufs_open(rom,&data));
 assert(!cnk_app_runtime_init(&app,&data,3,2));
 uint64_t now=1000000u;
 assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&sn));
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,CNK_UI_CONFIRM,(cnk_driver_input){0},&sn));
 assert(sn.state==CNK_FRONTEND_RACE);
 /* Never spend an item during countdown. */
 app.frontend.game.session.items.racers[0].held=CNK_WEAPON_TROPY_CLOCK;
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(app.frontend.game.session.items.racers[0].held==CNK_WEAPON_TROPY_CLOCK);
 for(int i=0;i<232;i++){
  now+=16667u;
  assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&sn));
 }
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,CNK_UI_ITEM,(cnk_driver_input){0},&sn));
 assert(sn.clock_seconds>0 && !sn.clock_affects_player);
 assert(sn.item_events[0]&CNK_ITEM_EVENT_CLOCK_ON);
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,CNK_UI_PAUSE,(cnk_driver_input){0},&sn));
 const float frozen=app.frontend.game.session.items.clock_seconds;
 for(int i=0;i<30;i++){
  now+=16667u;
  assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&sn));
 }
 assert(app.frontend.game.session.items.clock_seconds==frozen);
 /* Restart clears the global statuses. */
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,CNK_UI_RESTART,(cnk_driver_input){0},&sn));
 assert(sn.clock_seconds==0 && sn.candle_seconds==0);
 /* Re-enter running mode and let a rival cast candle onto player. */
 for(int i=0;i<232;i++){
  now+=16667u;
  assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&sn));
 }
 app.frontend.game.session.items.racers[1].held=CNK_WEAPON_CURSED_CANDLE;
 now+=16667u;
 assert(!cnk_app_runtime_tick(&app,now,0,(cnk_driver_input){0},&sn));
 assert(sn.candle_affects_player && sn.candle_seconds>0);
 assert(sn.item_events[1]&CNK_ITEM_EVENT_CANDLE_ON);
 cnk_app_runtime_close(&app);fufs_close(&data);
}
int main(int argc,char **argv){
 assert(argc==2);
 original_lang_proof(argv[1]);source_reviewed_sidecar();
 time_and_effects();native_road_clock_and_candle(argv[1]);
 puts("PASS: source LANG, Tropy Clock slowdown, Cursed Candle darkness, AI, timer, pause/restart, sidecar, integrity");
 return 0;
}
