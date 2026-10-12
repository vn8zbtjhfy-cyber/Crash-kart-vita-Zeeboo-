/* Vita motion adapter checked against a fake sensor API on PC.
 * This does NOT test the actual PS Vita accelerometer. */
#include "cnk_vita_motion.h"
#include <psp2/motion.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int start_calls=0,stop_calls=0,read_calls=0;
static int start_error=0,read_error=0;
static float measured_x=0.0f;
int sceMotionStartSampling(void){start_calls++;return start_error;}
int sceMotionStopSampling(void){stop_calls++;return 0;}
int sceMotionGetSensorState(SceMotionSensorState *p,int n){
 read_calls++;
 if(n!=1||!p||read_error)return -1;
 memset(p,0,sizeof(*p));
 p->accelerometer.x=measured_x;
 return 0;
}
int main(void){
 cnk_vita_motion m={0};
 float steer=0.42f;
 assert(cnk_vita_motion_read(&m,&steer)<0 && steer==0.42f);
 assert(cnk_vita_motion_start(NULL)<0);
 start_error=-1;
 assert(cnk_vita_motion_start(&m)<0);
 assert(start_calls==1 && stop_calls==0 && !m.sampling);
 start_error=0;read_error=1;
 assert(cnk_vita_motion_start(&m)<0);
 assert(start_calls==2 && stop_calls==1 && !m.sampling);
 read_error=0;measured_x=NAN;
 assert(cnk_vita_motion_start(&m)<0);
 assert(stop_calls==2 && !m.sampling);
 measured_x=0.2f;
 assert(!cnk_vita_motion_start(&m));
 assert(m.sampling && m.steering.calibrated);
 measured_x=0.85f;
 for(int i=0;i<16;i++)assert(!cnk_vita_motion_read(&m,&steer));
 assert(steer>0.99f && steer<=1.0f);
 read_error=1;steer=0.123f;
 assert(cnk_vita_motion_read(&m,&steer)<0 && steer==0.123f);
 assert(m.sampling); /* intermittent failure may recover */
 read_error=0;measured_x=0.2f;
 assert(!cnk_vita_motion_read(&m,&steer));
 assert(steer<1.0f && steer>=0);
 cnk_vita_motion_stop(&m);
 assert(!m.sampling && stop_calls==3);
 cnk_vita_motion_stop(&m);
 assert(stop_calls==3);
 assert(read_calls>0);
 puts("PASS: Vita motion adapter start/calibrate/read/stop and safe fallback");
 return 0;
}
