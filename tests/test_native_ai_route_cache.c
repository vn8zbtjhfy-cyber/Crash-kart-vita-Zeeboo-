#include "cnk_nav.h"
#include "cnk_drive.h"
#include "fufs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int almost(float a,float b){return fabsf(a-b)<0.0001f;}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 const uint32_t scenes[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 unsigned checks=0;
 for(unsigned t=0;t<12;t++){
  unsigned char *bytes=NULL;size_t n=0;
  assert(!fufs_extract(&vfs,scenes[t],&bytes,&n));
  cnk_scene scene={0};assert(!cnk_parse_scene(bytes,n,&scene));free(bytes);
  const cnk_sline *route=cnk_scene_line(&scene,"DRV0");
  assert(route&&route->points&&route->count>=3);
  cnk_drive_tuning tuning={0};cnk_drive_default_tuning(&tuning);
  cnk_ai_driver ai={0};assert(!cnk_ai_init(&ai,route,1,&tuning));
  float total=cnk_nav_length(route,1);
  assert(total>0.0f&&almost(ai.route_length,total));
  for(int closed=0;closed<2;closed++){
   float l=cnk_nav_length(route,closed);
   for(int i=-24;i<=64;i++){
    float f=(float)i/24.0f;
    cnk_vec3 old={0},cached={0};
    assert(!cnk_nav_sample(route,f,closed,&old));
    assert(!cnk_nav_sample_with_length(route,f,closed,l,&cached));
    assert(almost(old.x,cached.x)&&almost(old.y,cached.y)&&almost(old.z,cached.z));
    checks++;
   }
  }
  cnk_kart kart={0};assert(!cnk_kart_spawn(&kart,route->points[0],0.0f));
  cnk_driver_input controls={0};assert(!cnk_ai_controls(&ai,&kart,&tuning,&controls));
  assert(isfinite(controls.steer)&&isfinite(controls.throttle)&&isfinite(controls.brake));
  cnk_scene_free(&scene);
 }
 assert(checks==2136);
 cnk_sline invalid={0};cnk_vec3 point={0};
 assert(cnk_nav_sample_with_length(&invalid,0,1,2,&point)<0);
 fufs_close(&vfs);
 printf("PASS: 12 source DRV0 routes, %u legacy/cached samples identical, cached AI target length\n",checks);
 return 0;
}
