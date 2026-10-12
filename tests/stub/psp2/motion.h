/* Host-only imitation of the motion header. Not a Vita implementation. */
#ifndef CNK_HOST_PSP2_MOTION_H
#define CNK_HOST_PSP2_MOTION_H
typedef struct {float x,y,z,w;} SceFVector3;
typedef struct {
 SceFVector3 accelerometer;
 SceFVector3 gyro;
} SceMotionSensorState;
int sceMotionStartSampling(void);
int sceMotionStopSampling(void);
int sceMotionGetSensorState(SceMotionSensorState *state,int records);
#endif
