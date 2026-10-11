#include "cnk_race.h"
#include "cnk_nav.h"
#include "fufs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int synthetic_test(void){
 /* Check reverse direction, ordered checkpoints and exact finish count. */
 cnk_race_track t={0};t.gate_count=3;
 t.gates[0].a=(cnk_vec3){0,0,-1};t.gates[0].b=(cnk_vec3){0,0,1};
 t.gates[1].a=(cnk_vec3){10,0,-1};t.gates[1].b=(cnk_vec3){10,0,1};
 t.gates[2].a=(cnk_vec3){20,0,-1};t.gates[2].b=(cnk_vec3){20,0,1};
 for(int i=0;i<3;i++)t.gates[i].direction=-1; /* x increasing */
 cnk_race r;
 if(cnk_race_start(&r,&t,1,1000))return 1;
 cnk_vec3 a={0,0,0},b={21,0,0};
 /* Crossing gates 1 & 2 in correct order on same movement step. */
 int e=cnk_race_advance(&r,a,b,1010);
 if(e!=CNK_RACE_GATE || r.next_gate!=0)return 2;
 /* Reverse finish must be ignored. */
 a=(cnk_vec3){1,0,0};b=(cnk_vec3){-1,0,0};
 if(cnk_race_advance(&r,a,b,1020)||r.completed_laps)return 3;
 /* Make legitimate forward finish crossing. */
 a=(cnk_vec3){-1,0,0};b=(cnk_vec3){1,0,0};
 e=cnk_race_advance(&r,a,b,1030);
 if(e!=(CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH)||
    !r.finished||r.completed_laps!=1||r.last_lap_ms!=25 || r.finish_ms!=1025)return 4;
 if(cnk_race_advance(&r,a,b,1040)!=-1)return 5;
 /* Crossing split between two frames at x=10 must count once, not zero/two. */
 if(cnk_race_start(&r,&t,1,3000))return 6;
 a=(cnk_vec3){9,0,0};b=(cnk_vec3){10,0,0};
 if(cnk_race_advance(&r,a,b,3010)!=0)return 7;
 a=b;b=(cnk_vec3){11,0,0};
 if(cnk_race_advance(&r,a,b,3020)!=CNK_RACE_GATE || r.next_gate!=2)return 8;
 /* Going to the wrong checkpoint with the correct direction resets
  * progress but never awards an incomplete lap. */
 a=(cnk_vec3){-1,0,0};b=(cnk_vec3){1,0,0};
 if(cnk_race_advance(&r,a,b,3030)!=0 || r.next_gate!=1 || r.completed_laps)return 9;
 if(cnk_race_advance(&r,b,a,3040)!=0)return 10;
 /* Reversal of the monotonic clock is invalid. */
 if(cnk_race_advance(&r,a,b,3001)!=-2)return 11;
 return 0;
}
static int run_original(const cnk_scene *scene, const cnk_race_track *track){
 const cnk_sline *drv=cnk_scene_line(scene,"DRV0");
 if(!drv)return 11;
 cnk_race state;
 if(cnk_race_start(&state,track,2,1000))return 12;
 float start=track->gates[0].fraction+0.002f;
 cnk_vec3 prev;
 if(cnk_nav_sample(drv,start,1,&prev))return 13;
 for(uint32_t step=1;step<=12000;step++){
  float f=start+2.0f*(float)step/12000.0f;
  cnk_vec3 p;
  if(cnk_nav_sample(drv,f,1,&p))return 14;
  uint64_t now=1000+step*17u;
  int r=cnk_race_advance(&state,prev,p,now);
  if(r<0)return 15;
  prev=p;
  if(state.finished)break;
 }
 if(!state.finished||state.completed_laps!=2){
  fprintf(stderr,"did not complete original route: checkpoints=%u next=%u laps=%u\n",
          track->gate_count,state.next_gate,state.completed_laps);
  return 16;
 }
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 int syn=synthetic_test();if(syn){fprintf(stderr,"synthetic race traversal failed %d\n",syn);return 2;}
 fufs_file file;
 if(fufs_open(argv[1],&file))return 3;
 uint32_t full=0,excluded=0,menu=0,gates=0;
 for(uint32_t i=0;i<file.count;i++){
  unsigned char *data=NULL;size_t size=0;
  if(fufs_extract(&file,i,&data,&size)){fufs_close(&file);return 4;}
  if(size>=12 && !memcmp(data+8,"SCEN",4)){
   cnk_scene scene;
   if(cnk_parse_scene(data,size,&scene)){free(data);fufs_close(&file);return 5;}
   cnk_race_track track;
   int rc=cnk_race_track_build(&scene,&track);
   if(!rc){
    int run=run_original(&scene,&track);
    if(run){fprintf(stderr,"SCEN %u: traversal test %d\n",i,run);return 6;}
    gates+=track.gate_count;full++;
    printf("SCEN %u: %u ordered original LAP checkpoints, 2 laps passed\n",i,track.gate_count);
   }else if(i==289&&rc==-4){excluded++;printf("SCEN %u: ambiguous LAP segments explicitly rejected\n",i);}
   else if(i==181&&rc==-3){menu++;printf("SCEN %u: no DRV/LAP race course, skipped\n",i);}
   else{fprintf(stderr,"SCEN %u unexpected error %d\n",i,rc);return 7;}
   cnk_scene_free(&scene);
  }
  free(data);
 }
 fufs_close(&file);
 printf("CHECKPOINTS: validated=%u tracks=%u non-race=%u ambiguous=%u\n",gates,full,menu,excluded);
 if(full!=12||excluded!=1||menu!=1)return 8;
 return 0;
}
