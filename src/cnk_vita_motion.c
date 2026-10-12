#include "cnk_vita_motion.h"
#include <psp2/motion.h>
#include <string.h>

void cnk_vita_motion_stop(cnk_vita_motion *motion){
 if(!motion)return;
 if(motion->sampling)(void)sceMotionStopSampling();
 memset(motion,0,sizeof(*motion));
}
int cnk_vita_motion_start(cnk_vita_motion *motion){
 if(!motion)return -1;
 memset(motion,0,sizeof(*motion));
 if(sceMotionStartSampling()<0)return -2;
 motion->sampling=1;
 SceMotionSensorState sample={0};
 if(sceMotionGetSensorState(&sample,1)<0 ||
    cnk_motion_steer_calibrate(&motion->steering,
                               sample.accelerometer.x)){
  cnk_vita_motion_stop(motion);
  return -3;
 }
 return 0;
}
int cnk_vita_motion_read(cnk_vita_motion *motion,float *steering){
 if(!motion||!steering||!motion->sampling)return -1;
 SceMotionSensorState sample={0};
 if(sceMotionGetSensorState(&sample,1)<0)return -2;
 return cnk_motion_steer_sample(&motion->steering,
                                sample.accelerometer.x,steering);
}
