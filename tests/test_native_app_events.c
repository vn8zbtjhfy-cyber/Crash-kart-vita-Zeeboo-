#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void step(cnk_app_runtime *app,uint64_t time,uint32_t keys,
                 cnk_app_snapshot *out){
 assert(!cnk_app_runtime_tick(app,time,keys,(cnk_driver_input){0},out));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot s={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 step(&app,1000000,0,&s);assert(s.app_events==0);
 step(&app,1010000,CNK_UI_NEXT,&s);
 assert(s.app_events==CNK_APP_EVENT_TRACK_CHANGED&&s.selected_race==1);
 step(&app,1020000,CNK_UI_NEXT,&s);assert(!s.app_events);
 step(&app,1030000,0,&s);
 step(&app,1040000,CNK_UI_CONFIRM,&s);
 assert(s.app_events==(CNK_APP_EVENT_RACE_STARTED|CNK_APP_EVENT_COUNT_3) && s.state==CNK_FRONTEND_RACE);
 step(&app,1050000,CNK_UI_CONFIRM,&s);assert(!s.app_events);
 /* Checkpoint/lap/finish signals are carried without assigning any sounds. */
 app.frontend.game.last_race_events[0]=CNK_RACE_GATE|CNK_RACE_LAP;
 app.frontend.game.last_race_events[2]=CNK_RACE_FINISH;
 step(&app,1050000,0,&s);
 assert(s.race_events[0]==(CNK_RACE_GATE|CNK_RACE_LAP));
 assert(s.race_events[1]==0 && s.race_events[2]==CNK_RACE_FINISH);
 step(&app,1050000,0,&s);
 for(int i=0;i<CNK_MAX_RACERS;i++)assert(s.race_events[i]==0);
 step(&app,1060000,CNK_UI_PAUSE,&s);
 assert(s.app_events==CNK_APP_EVENT_PAUSED&&s.state==CNK_FRONTEND_PAUSE);
 step(&app,1070000,CNK_UI_PAUSE,&s);assert(!s.app_events);
 step(&app,1080000,0,&s);
 step(&app,1090000,CNK_UI_PAUSE,&s);
 assert(s.app_events==CNK_APP_EVENT_RESUMED&&s.state==CNK_FRONTEND_RACE);
 step(&app,1100000,0,&s);
 step(&app,1110000,CNK_UI_PAUSE,&s);
 assert(s.app_events==CNK_APP_EVENT_PAUSED);
 step(&app,1120000,0,&s);
 step(&app,1130000,CNK_UI_RESTART,&s);
 assert(s.state==CNK_FRONTEND_RACE);
 assert(s.app_events==(CNK_APP_EVENT_RACE_STARTED|CNK_APP_EVENT_RACE_RESTARTED|CNK_APP_EVENT_COUNT_3));
 assert(s.hud.lap<=s.hud.total_laps);
 step(&app,1140000,0,&s);
 step(&app,1150000,CNK_UI_PAUSE,&s);
 step(&app,1160000,0,&s);
 step(&app,1170000,CNK_UI_CANCEL,&s);
 assert(s.app_events==CNK_APP_EVENT_BACK_TO_SELECT && s.state==CNK_FRONTEND_SELECT);
 for(int i=0;i<CNK_MAX_RACERS;i++)assert(s.race_events[i]==0);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS: single-frame race/menu state signals and per-racer source checkpoint/lap/finish events; original audio identity remains unassigned.");
 return 0;
}
