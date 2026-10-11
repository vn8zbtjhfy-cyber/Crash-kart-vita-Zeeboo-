#include "cnk_ai_watchdog.h"
#include <math.h>
int cnk_ai_watchdog_step(cnk_ai_watchdog *watch,cnk_vec3 pos,
                         float dt,int in_control){
 if(!watch||!isfinite(dt)||dt<=0.0f||dt>0.1f||
    !isfinite(pos.x)||!isfinite(pos.y)||!isfinite(pos.z))return -1;
 if(!watch->initialized||!in_control){
  watch->anchor=pos;watch->still_seconds=0.0f;
  watch->initialized=!!in_control;
  return 0;
 }
 float dx=pos.x-watch->anchor.x,dz=pos.z-watch->anchor.z;
 if(!isfinite(dx)||!isfinite(dz))return -1;
 if(dx*dx+dz*dz>=CNK_AI_STUCK_RADIUS*CNK_AI_STUCK_RADIUS){
  watch->anchor=pos;watch->still_seconds=0.0f;return 0;
 }
 watch->still_seconds+=dt;
 if(watch->still_seconds+0.00001f<CNK_AI_STUCK_SECONDS)return 0;
 watch->anchor=pos;watch->still_seconds=0.0f;
 return 1;
}
