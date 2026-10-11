/* A visually mirrored camera can pass every collision and trajectory test.
 * Verify actual 3D world axes map to correct projected screen directions. */
#include "cnk_camera.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static float det3(const float m[16]){
 float a=m[0],b=m[4],c=m[8];
 float d=m[1],e=m[5],f=m[9];
 float g=m[2],h=m[6],i=m[10];
 return a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
}
int main(void){
 cnk_camera cam={0};
 assert(cnk_camera_build(&cam,(cnk_vec3){0,10,50},(cnk_vec3){0,10,0},
       1.1f,960.0f/544.0f,0.15f,8000)==0);
 float px=0,py=0,cx=0,cy=0,lx=0,ly=0,ux=0,uy=0;
 assert(cnk_camera_project(&cam,(cnk_vec3){0,10,0},&cx,&cy)==0);
 assert(cnk_camera_project(&cam,(cnk_vec3){5,10,0},&px,&py)==0);
 assert(cnk_camera_project(&cam,(cnk_vec3){-5,10,0},&lx,&ly)==0);
 assert(cnk_camera_project(&cam,(cnk_vec3){0,15,0},&ux,&uy)==0);
 assert(px>cx&&lx<cx&&uy>cy);
 assert(fabsf(det3(cam.view)-1.0f)<0.0001f);
 /* Also test a rotated viewpoint; scene must never flip handedness. */
 assert(cnk_camera_build(&cam,(cnk_vec3){10,12,50},(cnk_vec3){0,10,0},
       1.1f,960.0f/544.0f,0.15f,8000)==0);
 assert(fabsf(det3(cam.view)-1.0f)<0.0001f);
 puts("PASS: right-handed camera, no mirrored tracks/vehicles");
 return 0;
}
