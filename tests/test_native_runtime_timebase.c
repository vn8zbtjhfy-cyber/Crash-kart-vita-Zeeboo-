#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void tick(cnk_app_runtime *app,uint64_t time_us,uint32_t buttons,cnk_app_snapshot *s){
 assert(!cnk_app_runtime_tick(app,time_us,buttons,(cnk_driver_input){0},s));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 cnk_app_snapshot s={0};
 tick(&app,10000000,0,&s);assert(!app.last_delta_us);
 tick(&app,10100000,0,&s);assert(app.last_delta_us==100000);
 tick(&app,10080000,0,&s); /* small clock regression */
 assert(!app.last_delta_us&&app.last_clock_us==10100000);
 tick(&app,10120000,0,&s);
 assert(app.last_delta_us==20000&&app.last_clock_us==10120000);
 tick(&app,9000000,0,&s); /* large discontinuity: reset only */
 assert(!app.last_delta_us&&app.last_clock_us==9000000);
 tick(&app,9040000,0,&s);
 assert(app.last_delta_us==40000);
 tick(&app,9500000,CNK_UI_CONFIRM,&s); /* 460ms gap clipped to 250ms */
 assert(app.last_delta_us==250000 && s.state==CNK_FRONTEND_RACE);
 unsigned phase=s.start_phase_remaining_ms;
 tick(&app,9480000,0,&s); /* regression cannot advance 3-2-1 */
 assert(!app.last_delta_us && s.start_phase_remaining_ms==phase);
 tick(&app,9520000,0,&s);
 assert(app.last_delta_us==20000);
 assert(s.start_phase_remaining_ms<phase);
 uint64_t count=app.frontend.game.simulation_frames;
 tick(&app,8000000,0,&s);
 assert(!app.last_delta_us && app.frontend.game.simulation_frames==count);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS: backwards jitter, large clock rebase, capped 250ms, countdown and simulated steps never leap on rewind");
 return 0;
}
