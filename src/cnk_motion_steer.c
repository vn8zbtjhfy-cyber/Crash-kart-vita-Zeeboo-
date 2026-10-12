#include "cnk_motion_steer.h"
#include <math.h>

static float clamp_unit(float x){
 if(x>1.0f)return 1.0f;
 if(x< -1.0f)return -1.0f;
 return x;
}
static int valid_sample(float x){
 /* The Vita acceleration vector is expressed in units of g. Ignore
  * spikes or NaNs instead of steering suddenly into an obstacle. */
 return isfinite(x) && x>=-4.0f && x<=4.0f;
}
int cnk_motion_steer_calibrate(cnk_motion_steer *state,float acceleration_x){
 if(!state||!valid_sample(acceleration_x))return -1;
 *state=(cnk_motion_steer){acceleration_x,0.0f,1};
 return 0;
}
int cnk_motion_steer_sample(cnk_motion_steer *state,float acceleration_x,
                            float *steering){
 if(!state||!steering||!state->calibrated||
    !valid_sample(state->neutral_x)||!valid_sample(acceleration_x)||
    !isfinite(state->smoothed)||fabsf(state->smoothed)>1.0f)return -1;
 /* Normalized lateral tilt, with a rescaled deadband. These provisional
  * parameters are for controllability on Vita, NOT recovered Zeebo values. */
 float target=clamp_unit((acceleration_x-state->neutral_x)*1.6f);
 const float deadband=0.08f;
 if(fabsf(target)<=deadband)target=0.0f;
 else target=copysignf((fabsf(target)-deadband)/(1.0f-deadband),target);
 /* Per-display-frame filtering. A skipped sensor frame never advances
  * this filter; there is no timer and no effect on physics determinism. */
 float next=clamp_unit(state->smoothed+0.35f*(target-state->smoothed));
 state->smoothed=next;
 *steering=next;
 return 0;
}
