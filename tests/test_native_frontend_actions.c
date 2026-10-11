#include "cnk_frontend_actions.h"
#include "fufs.h"
#include <assert.h>
#include <stdio.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_frontend ui={0};assert(!cnk_frontend_init(&ui,&vfs,3,2));
 cnk_frontend_action_state state={0};
 cnk_driver_input drive={1,0,0};
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_NEXT,drive,200000));
 assert(ui.selected_race==1);
 for(int i=0;i<5;i++)assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_NEXT,drive,200000));
 assert(ui.selected_race==1); /* long press must not advance multiple tracks */
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_CONFIRM,drive,250000));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.game.simulation_frames==0);
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 int moved=cnk_frontend_dispatch(&ui,&state,CNK_UI_CONFIRM,drive,16667);
 assert(moved==0); /* holding/pressing accelerate cannot skip the countdown */
 for(int i=0;i<11;i++)assert(!cnk_frontend_dispatch(&ui,&state,0,drive,250000));
 moved=cnk_frontend_dispatch(&ui,&state,0,drive,250000);
 assert(moved>=0);
 if(!moved)moved=cnk_frontend_dispatch(&ui,&state,0,drive,16667);
 assert(moved>0);
 uint64_t n=ui.game.simulation_frames;
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_PAUSE,drive,250000));
 assert(ui.state==CNK_FRONTEND_PAUSE&&ui.game.simulation_frames==n);
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_PAUSE,drive,250000));
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_CONFIRM,drive,250000));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.game.simulation_frames==n);
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_PAUSE,drive,0));
 assert(ui.state==CNK_FRONTEND_PAUSE);
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_RESTART,drive,250000));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.game.simulation_frames==0);
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_PAUSE,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,0,drive,0));
 assert(!cnk_frontend_dispatch(&ui,&state,CNK_UI_CANCEL,drive,0));
 assert(ui.state==CNK_FRONTEND_SELECT&&!ui.game.loaded);
 assert(cnk_frontend_dispatch(&ui,&state,~CNK_UI_ALL,drive,0)<0);
 cnk_frontend_close(&ui);fufs_close(&vfs);
 puts("INPUT PASS: edge-triggered source track menu, acceleration, pause, restart, back");
 return 0;
}
