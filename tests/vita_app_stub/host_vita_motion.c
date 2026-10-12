/* Host-side fake motion device for the native app lifecycle test.
 * Tilt is disabled by default, and no physical Vita sensors are exercised. */
#include <psp2/motion.h>
#include <string.h>
int sceMotionStartSampling(void){return 0;}
int sceMotionStopSampling(void){return 0;}
int sceMotionGetSensorState(SceMotionSensorState *out,int count){
 if(!out||count!=1)return -1;
 memset(out,0,sizeof(*out));
 return 0;
}
