#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t clock_us;
static void tick(cnk_app_runtime *app,uint32_t buttons,cnk_app_snapshot *sn){
 clock_us+=16667u;
 assert(!cnk_app_runtime_tick(app,clock_us,buttons,(cnk_driver_input){0},sn));
}
static void use_weapon(cnk_app_runtime *app,cnk_app_snapshot *sn,
                       cnk_weapon weapon,uint32_t expected_event){
 cnk_game *g=&app->frontend.game;
 g->session.items.racers[0].held=weapon; /* controlled test equipment */
 tick(app,0,sn); /* arm edge trigger */
 tick(app,CNK_UI_ITEM,sn);
 assert(sn->app_events & CNK_APP_EVENT_ITEM_FIRED);
 assert(sn->item_events[0] & expected_event);
 assert(sn->player_items.held==CNK_WEAPON_NONE);
 tick(app,0,sn);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot sn={0};
 assert(!cnk_app_runtime_init(&app,&vfs,2,2));
 clock_us=1000000;
 assert(!cnk_app_runtime_tick(&app,clock_us,0,(cnk_driver_input){0},&sn));
 tick(&app,CNK_UI_CONFIRM,&sn);
 assert(sn.state==CNK_FRONTEND_RACE);
 cnk_game *g=&app.frontend.game;
 /* Countdown rejects premature projectile launch and queued item edges. */
 g->session.items.racers[0].held=CNK_WEAPON_MISSILE;
 tick(&app,CNK_UI_ITEM,&sn);
 assert(g->session.items.racers[0].held==CNK_WEAPON_MISSILE);
 assert(!g->session.items.missiles[0].active);
 for(int i=0;i<230;i++)tick(&app,0,&sn);
 assert(sn.start_phase==CNK_START_GO || sn.start_phase==CNK_START_RUNNING);
 /* First actual input after GO consumes the missile, never earlier. */
 tick(&app,CNK_UI_ITEM,&sn);
 assert(sn.item_events[0]&CNK_ITEM_EVENT_MISSILE_FIRED);
 assert(sn.app_events&CNK_APP_EVENT_MISSILE_FIRED);
 assert(sn.active_missiles==1);
 assert(g->session.items.missiles[0].active);
 float lifetime=g->session.items.missiles[0].lifetime;
 tick(&app,CNK_UI_PAUSE,&sn);
 assert(sn.state==CNK_FRONTEND_PAUSE && sn.active_missiles==1);
 for(int i=0;i<40;i++)tick(&app,0,&sn);
 assert(g->session.items.missiles[0].lifetime==lifetime);
 tick(&app,CNK_UI_PAUSE,&sn);
 assert(sn.state==CNK_FRONTEND_RACE);
 tick(&app,0,&sn);
 assert(g->session.items.missiles[0].lifetime<lifetime);
 use_weapon(&app,&sn,CNK_WEAPON_TNT,CNK_ITEM_EVENT_HAZARD_DROPPED);
 assert(sn.app_events==0 || sn.active_hazards>=1);
 assert(g->session.items.hazards[0].active &&
        g->session.items.hazards[0].kind==CNK_WEAPON_TNT);
 use_weapon(&app,&sn,CNK_WEAPON_OIL,CNK_ITEM_EVENT_HAZARD_DROPPED);
 assert(g->session.items.hazards[1].active &&
        g->session.items.hazards[1].kind==CNK_WEAPON_OIL);
 /* Restart wipes active projectiles, traps, statuses and inventory. */
 tick(&app,CNK_UI_PAUSE,&sn);tick(&app,0,&sn);
 tick(&app,CNK_UI_RESTART,&sn);
 assert(sn.state==CNK_FRONTEND_RACE && sn.start_phase==CNK_START_THREE);
 assert(sn.player_items.held==CNK_WEAPON_NONE);
 assert(sn.active_missiles==0 && sn.active_hazards==0);
 for(int i=0;i<CNK_ITEM_MAX_MISSILES;i++)assert(!g->session.items.missiles[i].active);
 for(int i=0;i<CNK_ITEM_MAX_HAZARDS;i++)assert(!g->session.items.hazards[i].active);
 cnk_app_runtime_close(&app);
 /* Verify the source-ID placement parser accepts every NEW reward and
  * rejects malformed names without mutating the last valid layout. */
 char path[]="/tmp/cnk48_weapons_layout_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *f=fdopen(fd,"wb");assert(f);
 const uint32_t source_id=17;
 fprintf(f,"%u\tCRATE\t0\t0\t1\t1\tMISSILE\n",source_id);
 fprintf(f,"%u\tCRATE\t0\t0\t2\t1\tTNT\n",source_id);
 fprintf(f,"%u\tCRATE\t0\t0\t3\t1\tOIL\n",source_id);
 assert(!fclose(f));
 cnk_item_layout layout={0};assert(!cnk_item_layout_read(path,&layout));
 assert(layout.count==3);
 assert(layout.entries[0].pickup.reward==CNK_WEAPON_MISSILE);
 assert(layout.entries[1].pickup.reward==CNK_WEAPON_TNT);
 assert(layout.entries[2].pickup.reward==CNK_WEAPON_OIL);
 f=fopen(path,"wb");assert(f);
 fprintf(f,"%u\tCRATE\t0\t0\t3\t1\tFAKE\n",source_id);
 assert(!fclose(f));
 assert(cnk_item_layout_read(path,&layout)<0);
 assert(layout.count==3);
 assert(!unlink(path));
 fufs_close(&vfs);
 puts("PASS: original track countdown, missile fired on L, pause freeze, TNT/oil UI edge, restart reset, strict weapon TSV");
 return 0;
}
