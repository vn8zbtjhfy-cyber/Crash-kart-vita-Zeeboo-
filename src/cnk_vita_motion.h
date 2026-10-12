#ifndef CNK_VITA_MOTION_H
#define CNK_VITA_MOTION_H
#include "cnk_motion_steer.h"

/* Optional native accelerometer adapter. Never starts automatically.
 * Motion errors are nonfatal: the caller retains normal D-pad/stick input. */
typedef struct {
 cnk_motion_steer steering;
 int sampling;
} cnk_vita_motion;
int cnk_vita_motion_start(cnk_vita_motion *motion);
int cnk_vita_motion_read(cnk_vita_motion *motion,float *steering);
void cnk_vita_motion_stop(cnk_vita_motion *motion);
#endif
