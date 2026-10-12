#include "cnk_vita_controls.h"
#include <psp2/ctrl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
static SceCtrlData sample={0,128,128};
static int fail_read=0;
int sceCtrlSetSamplingMode(int mode){return mode==SCE_CTRL_MODE_ANALOG?0:-1;}
int sceCtrlPeekBufferPositive(int port,SceCtrlData *pad,int count){
 if(port||count!=1)return -1;
 if(fail_read)return fail_read<0?-1:0;
 *pad=sample;return 1;
}
int main(void){
 assert(cnk_vita_controls_init()==0);
 cnk_driver_input input={0};uint32_t buttons=0;int exit_req=1;
 assert(cnk_vita_controls_read_actions(&input,&buttons,&exit_req)==0);
 assert(input.steer==0 && input.throttle==0 && input.brake==0 && buttons==0 && !exit_req);
 sample.buttons=SCE_CTRL_CROSS|SCE_CTRL_LEFT|SCE_CTRL_START;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(input.throttle==1 && input.brake==0 && input.steer==-1);
 assert((buttons&(CNK_UI_CONFIRM|CNK_UI_PREVIOUS|CNK_UI_PAUSE)) ==
        (CNK_UI_CONFIRM|CNK_UI_PREVIOUS|CNK_UI_PAUSE));
 assert(!exit_req);
 sample.buttons=SCE_CTRL_RTRIGGER;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==CNK_UI_DRIFT);
 sample.buttons=SCE_CTRL_LTRIGGER;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==CNK_UI_ITEM);
 sample.buttons=SCE_CTRL_SELECT;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==CNK_UI_LANGUAGE && !exit_req);
 sample.buttons=SCE_CTRL_SELECT|SCE_CTRL_START;
 assert(!cnk_vita_controls_read(&input,&exit_req)&&exit_req);
 sample.buttons=SCE_CTRL_CIRCLE|SCE_CTRL_TRIANGLE|SCE_CTRL_SQUARE;
 sample.lx=255;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(input.steer==1 && input.brake==1 && input.throttle==0);
 assert(buttons==(CNK_UI_CANCEL|CNK_UI_RESTART|CNK_UI_LAPS|CNK_UI_NEXT));
 assert(!exit_req);
 /* Stick works in menus without relying on a D-pad press. */
 sample.buttons=0;sample.lx=0;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(input.steer==-1 && buttons==CNK_UI_PREVIOUS);
 sample.lx=190;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==0 && input.steer>0.35f && input.steer<0.55f);
 sample.lx=143;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==0 && input.steer==0);
 sample.lx=145;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(input.steer>0 && input.steer<0.04f);
 sample.lx=255;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==CNK_UI_NEXT && input.steer==1);
 sample.buttons=SCE_CTRL_LEFT;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(buttons==CNK_UI_PREVIOUS && input.steer==-1);
 /* Native nonblocking pad polls sometimes have no new input sample.
  * Neutralize promptly; never inherit throttle or a menu confirmation. */
 fail_read=1;
 for(int i=0;i<7;i++){
  input.throttle=1;input.steer=-1;buttons=CNK_UI_CONFIRM;exit_req=1;
  assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
  assert(input.throttle==0 && input.brake==0 && input.steer==0);
  assert(buttons==0 && !exit_req);
 }
 assert(cnk_vita_controls_read_actions(&input,&buttons,&exit_req)<0);
 fail_read=0;sample.buttons=SCE_CTRL_CROSS;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 assert(input.throttle==1 && (buttons&CNK_UI_CONFIRM));
 /* One empty poll after recovery is again tolerated. */
 fail_read=1;
 assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 fail_read=-1;
 assert(cnk_vita_controls_read_actions(&input,&buttons,&exit_req)<0);
 assert(cnk_vita_controls_read_actions(&input,NULL,&exit_req)<0);
 assert(!cnk_vita_controls_init());
 fail_read=1;assert(!cnk_vita_controls_read_actions(&input,&buttons,&exit_req));
 puts("VITA INPUT PASS: physical controls, analog, transient missing samples, error path");
 return 0;
}
