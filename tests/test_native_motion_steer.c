#include "cnk_motion_steer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void){
 cnk_motion_steer state={0};
 float steering=0.75f;
 assert(cnk_motion_steer_sample(&state,0,&steering)<0);
 assert(steering==0.75f);
 assert(cnk_motion_steer_calibrate(NULL,0)<0);
 assert(cnk_motion_steer_calibrate(&state,NAN)<0);
 assert(cnk_motion_steer_calibrate(&state,INFINITY)<0);
 assert(!cnk_motion_steer_calibrate(&state,0.10f));
 assert(state.calibrated && state.neutral_x==0.10f);
 for(int i=0;i<10;i++)assert(!cnk_motion_steer_sample(&state,0.10f,&steering));
 assert(steering==0.0f);
 /* Tiny jitter within deadband never introduces unintended steering. */
 assert(!cnk_motion_steer_sample(&state,0.12f,&steering));
 assert(steering==0.0f);
 for(int i=0;i<15;i++)assert(!cnk_motion_steer_sample(&state,0.8f,&steering));
 assert(steering>0.99f && steering<=1.0f);
 for(int i=0;i<30;i++)assert(!cnk_motion_steer_sample(&state,-0.7f,&steering));
 assert(steering< -0.99f && steering>=-1.0f);
 cnk_motion_steer prior=state;
 float sentinel=0.19f;
 assert(cnk_motion_steer_sample(&state,NAN,&sentinel)<0);
 assert(cnk_motion_steer_sample(&state,5.0f,&sentinel)<0);
 assert(sentinel==0.19f && !memcmp(&prior,&state,sizeof(state)));
 assert(!cnk_motion_steer_calibrate(&state,-0.7f));
 assert(!cnk_motion_steer_sample(&state,-0.7f,&steering));
 assert(steering==0);
 puts("PASS: opt-in tilt calibration, filtered steering, jitter, error recovery");
 return 0;
}
