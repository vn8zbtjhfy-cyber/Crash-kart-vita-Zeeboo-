#ifndef CNK_MOTION_STEER_H
#define CNK_MOTION_STEER_H

/* Original independent Vita tilt-steering option inspired by published
 * Zeebo Boomerang behavior, NOT the original BREW accelerometer math.
 * Neutral is learned when the user enables tilt steering. */
typedef struct {
 float neutral_x;
 float smoothed;
 int calibrated;
} cnk_motion_steer;

int cnk_motion_steer_calibrate(cnk_motion_steer *state,float acceleration_x);
/* Read signed lateral acceleration (Vita sensor units). Returns 0 when
 * valid, <0 without changing state/output on invalid input. */
int cnk_motion_steer_sample(cnk_motion_steer *state,float acceleration_x,
                            float *steering);
#endif
