#include "cnk_launch.h"
#include <math.h>

void cnk_launch_reset(cnk_launch_state *s){
 if(s)*s=(cnk_launch_state){0};
}
int cnk_launch_step(cnk_launch_state *s,cnk_start_phase phase,
                    uint32_t remaining,uint32_t dt,float throttle,
                    uint32_t events){
 if(!s||!isfinite(throttle)||throttle<0.0f||throttle>1.0f||
    phase<CNK_START_IDLE||phase>CNK_START_RUNNING||
    ((phase==CNK_START_ONE||phase==CNK_START_TWO||
      phase==CNK_START_THREE)&&
     (!remaining||remaining>CNK_START_LIGHT_US)))return -1;
 cnk_launch_state n=*s;
 const int high=throttle>=0.80f;
 /* Only a low-to-high edge STARTING within ONE qualifies. A press during
  * TWO that happens to cross the boundary within a frame is rejected. */
 if(!n.consumed && phase==CNK_START_ONE){
  if(!high)n.pressed_in_one=0;
  if(high&&!n.previous_high){n.pressed_in_one=1;n.charged_us=0;}
  if(high&&n.pressed_in_one){
   uint32_t spent=dt<remaining?dt:remaining;
   if(spent>CNK_START_LIGHT_US-n.charged_us)
    n.charged_us=CNK_START_LIGHT_US;
   else n.charged_us+=spent;
  }
 }
 int award=0;
 if(events&CNK_START_EVENT_GO){
  award=(!n.consumed && high && n.pressed_in_one &&
          n.charged_us>=CNK_LAUNCH_HOLD_US);
  n.consumed=1; /* a second GO or forged event cannot farm boosts */
 }
 n.previous_high=high;
 *s=n;
 return award;
}
