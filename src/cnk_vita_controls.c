#include "cnk_vita_controls.h"
#include <psp2/ctrl.h>
#include <string.h>

/* A rescaled dead zone is continuous at its boundary: tiny controller
 * noise cannot abruptly turn the kart by 12% when steering begins. */
static float deadzone(float v){
 const float threshold=0.12f;
 if(v<=threshold && v>=-threshold)return 0;
 if(v>1.0f)v=1.0f;
 if(v< -1.0f)v=-1.0f;
 return v>0 ? (v-threshold)/(1.0f-threshold) :
              (v+threshold)/(1.0f-threshold);
}
/* A single nonblocking peek may report no new sample even with a healthy
 * controller. Use neutral input briefly instead of quitting the race. A real
 * SDK error or eight consecutive empty polls still aborts safely. */
static unsigned empty_polls=0;
int cnk_vita_controls_init(void){
 empty_polls=0;
 return sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
}
static int read_pad(cnk_driver_input *out,uint32_t *ui_bits,int *exit_requested){
 if(!out)return -1;
 /* Never reuse acceleration, steering, or a held UI event when a sample is
  * absent; synthetic neutral frames cannot activate a menu item. */
 memset(out,0,sizeof(*out));
 if(ui_bits)*ui_bits=0;
 if(exit_requested)*exit_requested=0;
 SceCtrlData pad;
 memset(&pad,0,sizeof(pad));
 int received=sceCtrlPeekBufferPositive(0,&pad,1);
 if(received<0)return -2;
 if(received==0){
  if(empty_polls<8u)empty_polls++;
  return empty_polls<8u?0:-2;
 }
 empty_polls=0;
 out->steer=deadzone(((float)pad.lx-128.0f)/127.0f);
 if(out->steer>1)out->steer=1;
 if(out->steer< -1)out->steer=-1;
 if(pad.buttons&SCE_CTRL_LEFT)out->steer=-1;
 if(pad.buttons&SCE_CTRL_RIGHT)out->steer=1;
 out->throttle=(pad.buttons&SCE_CTRL_CROSS)?1.0f:0.0f;
 out->brake=(pad.buttons&SCE_CTRL_SQUARE)?1.0f:0.0f;
 /* Provisional Vita binding, NOT a claim about original Zeebo controls.
  * Frontend consumes these only in its appropriate state, so CROSS still
  * accelerates in races rather than repeatedly selecting a menu option. */
 if(ui_bits){
  uint32_t bits=0;
  /* Menus may be controlled with either the D-pad or the analog stick.
   * Only strong analog deflection selects a direction, and the frontend's
   * existing button-edge detector requires returning to neutral before a
   * second selection. D-pad always takes priority. */
  if(pad.buttons&SCE_CTRL_LEFT)bits|=CNK_UI_PREVIOUS;
  else if(pad.buttons&SCE_CTRL_RIGHT)bits|=CNK_UI_NEXT;
  else if(pad.lx<=44)bits|=CNK_UI_PREVIOUS;
  else if(pad.lx>=212)bits|=CNK_UI_NEXT;
  if(pad.buttons&SCE_CTRL_CROSS)bits|=CNK_UI_CONFIRM;
  if(pad.buttons&SCE_CTRL_CIRCLE)bits|=CNK_UI_CANCEL;
  if(pad.buttons&SCE_CTRL_START)bits|=CNK_UI_PAUSE;
  if(pad.buttons&SCE_CTRL_TRIANGLE)bits|=CNK_UI_RESTART;
  if(pad.buttons&SCE_CTRL_SQUARE)bits|=CNK_UI_LAPS;
  if(pad.buttons&SCE_CTRL_RTRIGGER)bits|=CNK_UI_DRIFT;
  if(pad.buttons&SCE_CTRL_LTRIGGER)bits|=CNK_UI_ITEM;
  if(pad.buttons&SCE_CTRL_SELECT)bits|=CNK_UI_LANGUAGE;
  *ui_bits=bits;
 }
 if(exit_requested)*exit_requested=(pad.buttons&SCE_CTRL_START)&&
                                  (pad.buttons&SCE_CTRL_SELECT);
 return 0;
}
int cnk_vita_controls_read(cnk_driver_input *out,int *exit_requested){
 return read_pad(out,NULL,exit_requested);
}
int cnk_vita_controls_read_actions(cnk_driver_input *out,
                                   uint32_t *ui_buttons,
                                   int *exit_requested){
 if(!ui_buttons)return -1;
 return read_pad(out,ui_buttons,exit_requested);
}
