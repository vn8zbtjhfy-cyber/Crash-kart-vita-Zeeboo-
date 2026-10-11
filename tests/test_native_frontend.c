#include "fufs.h"
#include "cnk_frontend.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_frontend ui={0};
 assert(cnk_frontend_init(&ui,&vfs,3,2)==0);
 assert(ui.state==CNK_FRONTEND_SELECT);
 assert(ui.catalog.count==14);
 assert(ui.catalog.race_count==12);
 assert(cnk_frontend_selected(&ui));
 uint32_t initial=cnk_frontend_selected(&ui)->vfs_index;
 assert(cnk_frontend_select(&ui,-1)==0);
 assert(ui.selected_race==11);
 assert(cnk_frontend_select(&ui,1)==0);
 assert(cnk_frontend_selected(&ui)->vfs_index==initial);
 assert(cnk_frontend_select(&ui,0)<0);
 assert(cnk_frontend_start(&ui)==0);
 assert(ui.state==CNK_FRONTEND_RACE&&ui.game.loaded);
 assert(ui.game.source_spawn_enabled);
 assert(ui.game.scene_index==initial);
 assert(cnk_frontend_select(&ui,1)<0);
 assert(cnk_frontend_start(&ui)<0);
 cnk_driver_input throttle={1,0,0};
 /* Cars and timer remain frozen for the entire three-signal start. */
 for(int i=0;i<179;i++)assert(cnk_frontend_update(&ui,throttle,16667)==0);
 assert(ui.game.simulation_frames==0 && ui.game.session.now_ms==0);
 assert(cnk_frontend_update(&ui,throttle,16667)>=0);
 for(int i=0;i<40;i++)assert(cnk_frontend_update(&ui,throttle,16667)>=0);
 uint64_t before=ui.game.simulation_frames;
 assert(before>0);
 cnk_game_hud hud;
 assert(!cnk_game_hud_read(&ui.game,&hud));
 assert(hud.competitors==3 && hud.total_laps==2);
 assert(cnk_frontend_toggle_pause(&ui)==0);
 assert(ui.state==CNK_FRONTEND_PAUSE && ui.game.paused);
 for(int i=0;i<30;i++)assert(cnk_frontend_update(&ui,throttle,16667)==0);
 assert(ui.game.simulation_frames==before);
 assert(cnk_frontend_toggle_pause(&ui)==0);
 assert(ui.state==CNK_FRONTEND_RACE && !ui.game.paused);
 assert(cnk_frontend_update(&ui,throttle,16667)>0);
 assert(ui.game.simulation_frames>before);
 assert(cnk_frontend_restart(&ui)==0);
 assert(ui.game.loaded && ui.game.simulation_frames==0 && ui.starts==2);
 assert(ui.game.source_spawn_enabled);
 /* Result transition is based on actual game HUD data, not on arbitrary lap
  * guesses in the UI layer. Set a controlled finished flag to test wiring. */
 /* A restarted race also must complete the start sequence. */
 for(int i=0;i<181;i++)assert(cnk_frontend_update(&ui,throttle,16667)>=0);
 ui.game.session.racers[0].race.finished=1;
 assert(cnk_frontend_refresh(&ui)==0);
 assert(ui.state==CNK_FRONTEND_RESULTS);
 assert(cnk_frontend_update(&ui,throttle,16667)==0);
 assert(ui.game.simulation_frames>0);
 assert(cnk_frontend_toggle_pause(&ui)<0);
 assert(cnk_frontend_back_to_select(&ui)==0);
 assert(!ui.game.loaded && ui.state==CNK_FRONTEND_SELECT);
 assert(cnk_frontend_select(&ui,-1)==0);
 assert(cnk_frontend_start(&ui)==0);
 assert(ui.game.scene_index==cnk_frontend_selected(&ui)->vfs_index);
 cnk_frontend_close(&ui);
 assert(ui.state==CNK_FRONTEND_EMPTY);
 cnk_frontend_close(&ui);
 fufs_close(&vfs);
 puts("FRONTEND PASS: source scene selection, STRT, physics, pause, restart and results");
 return 0;
}
