#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int tick(cnk_app_runtime *app,uint64_t us,uint32_t buttons,
                cnk_app_snapshot *out){
 return cnk_app_runtime_tick(app,us,buttons,
                            (cnk_driver_input){1.0f,0,0},out);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!tick(&app,1000000u,0,&snap));
 assert(snap.state==CNK_FRONTEND_SELECT&&snap.available_races==12);
 assert(snap.rendered_frames==1&&snap.simulated_steps==0);
 uint32_t source=snap.source_resource_id;
 assert(!tick(&app,1010000u,CNK_UI_NEXT,&snap));
 assert(snap.selected_race==1 && snap.source_resource_id!=source);
 assert(!tick(&app,1020000u,CNK_UI_NEXT,&snap));
 assert(snap.selected_race==1); /* edge triggers, holding does not repeat */
 assert(!tick(&app,1030000u,0,&snap));
 assert(!tick(&app,1040000u,CNK_UI_PREVIOUS,&snap));
 assert(snap.selected_race==0&&snap.source_resource_id==source);
 assert(!tick(&app,1050000u,0,&snap));
 assert(!tick(&app,1060000u,CNK_UI_CONFIRM,&snap));
 assert(snap.state==CNK_FRONTEND_RACE&&snap.has_hud&&snap.simulated_steps==0);
 assert(app.frontend.game.source_spawn_enabled);
 assert(app.frontend.game.session.racer_count==3);
 assert(!tick(&app,1076667u,CNK_UI_CONFIRM,&snap));
 assert(snap.state==CNK_FRONTEND_RACE&&snap.simulated_steps==0);
 assert(snap.start_phase==CNK_START_THREE && snap.hud.elapsed_ms==0);
 uint64_t frames=app.frontend.game.simulation_frames;
 assert(!tick(&app,1093334u,CNK_UI_PAUSE,&snap));
 assert(snap.state==CNK_FRONTEND_PAUSE&&app.frontend.game.simulation_frames==frames);
 assert(!tick(&app,3500000u,CNK_UI_PAUSE,&snap));
 assert(snap.state==CNK_FRONTEND_PAUSE);
 assert(!tick(&app,3510000u,0,&snap));
 assert(!tick(&app,3520000u,CNK_UI_PAUSE,&snap));
 assert(snap.state==CNK_FRONTEND_RACE);
 assert(!tick(&app,3510000u,0,&snap)); /* clock regression does not underflow */
 assert(app.last_delta_us==0u);
 assert(!tick(&app,3600000u,0,&snap));
 assert(app.last_delta_us==80000u); /* ignore regressed clock; 3.60 - 3.52 s */
 assert(!tick(&app,4600000u,CNK_UI_PAUSE,&snap));
 assert(app.last_delta_us==250000u&&snap.state==CNK_FRONTEND_PAUSE);
 assert(!tick(&app,4610000u,0,&snap));
 assert(!tick(&app,4620000u,CNK_UI_CANCEL,&snap));
 assert(snap.state==CNK_FRONTEND_SELECT&&!snap.has_hud);
 assert(!tick(&app,4630000u,0,&snap));
 assert(!tick(&app,4640000u,CNK_UI_CONFIRM,&snap));
 assert(snap.state==CNK_FRONTEND_RACE&&snap.has_hud);
 assert(app.frontend.game.simulation_frames==0); /* clean reload */
 cnk_app_runtime_close(&app);
 assert(app.initialized==0);
 fufs_close(&vfs);
 puts("PASS: native game runtime initialization, original circuit selection, 60Hz ticking, pause/restart lifecycle, clock discontinuities and resource cleanup.");
 return 0;
}
