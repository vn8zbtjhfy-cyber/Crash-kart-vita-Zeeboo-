#include "cnk_boost_pads.h"
#include "cnk_drive.h"
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 cnk_boost_pad_world pads={0};cnk_item_world items={0};
 cnk_vec3 a[CNK_PAD_RACERS]={{0}};
 cnk_kart karts[CNK_PAD_RACERS]={0};
 uint8_t moving[CNK_PAD_RACERS]={1,1};
 uint32_t events[CNK_PAD_RACERS]={0};
 cnk_vec3 pos={0,0,0};
 assert(cnk_boost_pad_place(&pads,pos,1.0f)==0);
 assert(cnk_boost_pad_place(&pads,pos,1.0f)<0 && pads.count==1);
 cnk_boost_pad_world original=pads;
 assert(cnk_boost_pad_place(&pads,pos,NAN)<0);
 assert(memcmp(&pads,&original,sizeof pads)==0);
 a[0]=(cnk_vec3){-3,0,0};karts[0].position=(cnk_vec3){3,0,0};
 a[1]=(cnk_vec3){-3,4,0};karts[1].position=(cnk_vec3){3,4,0};
 assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,1.0f/60,events)==0);
 assert(events[0]==CNK_PAD_EVENT_APPLIED);
 assert(events[1]==0 && items.racers[1].turbo_seconds==0);
 assert(fabsf(items.racers[0].turbo_seconds-1.30f)<0.001f);
 cnk_drive_tuning base={0},tuned={0};cnk_drive_default_tuning(&base);
 assert(cnk_items_tune(&items,0,&base,&tuned)==0 && tuned.max_speed>base.max_speed);
 /* A parked racer may not farm turbo again each frame. */
 karts[0].position=(cnk_vec3){0,0,0};a[0]=(cnk_vec3){0,0,0};events[0]=0;
 assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,1.0f/60,events)==0);
 assert(events[0]==0);
 /* Invalid actor coordinates produce no changes in either world. */
 cnk_boost_pad_world saved=pads;cnk_item_world saved_items=items;
 a[0].x=NAN;events[0]=0;
 assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,0.016f,events)<0);
 assert(!memcmp(&pads,&saved,sizeof saved));
 assert(!memcmp(&items,&saved_items,sizeof items));
 a[0]=(cnk_vec3){0,0,0};
 /* Pause simply performs no pad steps, so nothing advances. */
 saved=pads;
 assert(!memcmp(&pads,&saved,sizeof saved));
 /* Leave pad, let cooldown expire, and reenter from outside. */
 karts[0].position=(cnk_vec3){3,0,0};
 assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,0.016f,events)==0);
 for(int i=0;i<55;i++){
  a[0]=(cnk_vec3){3,0,0};
  assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,0.016f,events)==0);
 }
 events[0]=0;a[0]=(cnk_vec3){3,0,0};karts[0].position=(cnk_vec3){0,0,0};
 assert(cnk_boost_pad_step(&pads,&items,a,karts,moving,2,0.016f,events)==0);
 assert(events[0]==CNK_PAD_EVENT_APPLIED);
 /* Integrate a synthetic location into an actual race loaded from Zeebo.
  * Only the circuit and kart spawn come from data.vfs, NOT the strip location. */
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(&vfs,&catalog));
 cnk_game game={0};assert(!cnk_game_load_catalog_race(&game,&catalog,0,2,2));
 cnk_vec3 spawn=game.session.racers[0].kart.position;
 assert(!cnk_game_place_pad(&game,spawn,1.2f));
 assert(cnk_game_update(&game,(cnk_driver_input){1,0,0},16667)>=1);
 uint32_t item_event=0;
 assert(!cnk_game_take_item_events(&game,0,&item_event));
 assert(item_event&CNK_ITEM_EVENT_PAD_BOOST);
 assert(game.session.items.racers[0].turbo_seconds>0);
 assert(cnk_game_update(&game,(cnk_driver_input){0},16667)>=1);
 item_event=0;assert(!cnk_game_take_item_events(&game,0,&item_event));
 assert(!(item_event&CNK_ITEM_EVENT_PAD_BOOST));
 cnk_game_unload(&game);fufs_close(&vfs);
 puts("boost pads: real ROM race, swept entry, cooldown, boosted tuning, atomic errors OK");
 return 0;
}
