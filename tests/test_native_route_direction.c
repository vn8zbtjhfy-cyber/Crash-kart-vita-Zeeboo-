#include "cnk_route_direction.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static cnk_vec3 xyz(float x,float z){cnk_vec3 p={x,0,z};return p;}
int main(void){
 cnk_vec3 points[]={xyz(0,0),xyz(100,0),xyz(100,100),xyz(0,100)};
 cnk_sline route={0};route.points=points;route.count=4;
 cnk_route_direction direction={0};
 float x=70.0f,z=0.0f,dt=1.0f/60.0f;
 /* Single jolt and brief reversal do not trip the warning. */
 for(int i=0;i<20;i++){
  cnk_vec3 before=xyz(x,z);x-=0.16f;
  assert(!cnk_route_direction_step(&direction,&route,before,xyz(x,z),dt));
  assert(!direction.wrong_way);
 }
 /* Sideways slipping is not driving against source path. */
 for(int i=0;i<30;i++){
  cnk_vec3 before=xyz(x,z);z+=0.10f;
  assert(!cnk_route_direction_step(&direction,&route,before,xyz(x,z),dt));
  assert(!direction.wrong_way);
 }
 z=0;
 /* Warning requires sustained backward translation, not facing backwards. */
 for(int i=0;i<60;i++){
  cnk_vec3 before=xyz(x,z);x-=0.16f;
  assert(!cnk_route_direction_step(&direction,&route,before,xyz(x,z),dt));
 }
 assert(direction.wrong_way);
 /* Stop keeps warning; no movement isn't proof of direction. */
 for(int i=0;i<30;i++)assert(!cnk_route_direction_step(&direction,&route,xyz(x,z),xyz(x,z),dt));
 assert(direction.wrong_way);
 /* Forward motion for <350ms should not blink alert off. */
 for(int i=0;i<15;i++){
  cnk_vec3 before=xyz(x,z);x+=0.16f;
  assert(!cnk_route_direction_step(&direction,&route,before,xyz(x,z),dt));
 }
 assert(direction.wrong_way);
 for(int i=0;i<20;i++){
  cnk_vec3 before=xyz(x,z);x+=0.16f;
  assert(!cnk_route_direction_step(&direction,&route,before,xyz(x,z),dt));
 }
 assert(!direction.wrong_way);
 /* Deep off-route => deliberately unknown, not a false alarm. */
 direction.wrong_way=1;
 assert(!cnk_route_direction_step(&direction,&route,xyz(50,50),xyz(49.9f,50),dt));
 assert(!direction.wrong_way);
 /* Invalid arguments are rejected without mutating state. */
 direction.reverse_seconds=.5f;
 cnk_route_direction prev=direction;
 assert(cnk_route_direction_step(&direction,&route,xyz(NAN,0),xyz(2,0),dt)<0);
 assert(!memcmp(&direction,&prev,sizeof(prev)));
 assert(cnk_route_direction_step(&direction,&route,xyz(1,0),xyz(2,0),.2f)<0);
 assert(!memcmp(&direction,&prev,sizeof(prev)));
 /* A discontinuous teleport is never counted as driving backwards. */
 assert(!cnk_route_direction_step(&direction,&route,xyz(80,0),xyz(35,0),dt));
 assert(direction.reverse_seconds==prev.reverse_seconds);
 /* The original route may use a distinct vertical baseline from runtime
  * physics. A vertical offset must NEVER suppress horizontal matching. */
 cnk_route_direction altitude={0};
 for(int i=0;i<60;i++){
  float current=75.0f-i*.2f;
  cnk_vec3 before={current,999.0f,0.0f};
  cnk_vec3 after={current-.2f,999.0f,0.0f};
  assert(!cnk_route_direction_step(&altitude,&route,before,after,dt));
 }
 assert(altitude.wrong_way);
 puts("PASS: sustained DRV0 reverse alert, hesitation, sideways drift, off-route suppression, invalid input, teleports");
}
