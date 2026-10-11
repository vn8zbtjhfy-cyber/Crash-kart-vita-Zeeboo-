#include "cnk_drive.h"
#include "fufs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_synthetic(void){
 cnk_drive_tuning t;cnk_drive_default_tuning(&t);
 if(cnk_drive_validate(&t))return 1;
 cnk_kart kart;
 if(cnk_kart_spawn(&kart,(cnk_vec3){0,1,0},0))return 2;
 cnk_driver_input gas={1,0,0};
 for(unsigned i=0;i<300;i++)if(cnk_drive_step(&kart,&t,gas,1.0f/60.0f,NULL))return 3;
 if(kart.speed<20||kart.speed>t.max_speed||fabsf(kart.position.x)>0.01f||kart.position.z<30)return 4;
 if(cnk_drive_step(&kart,&t,gas,0.2f,NULL)!=-1)return 5;
 if(cnk_drive_step(&kart,&t,(cnk_driver_input){NAN,0,0},0.016f,NULL)!=-1)return 6;
 if(cnk_drive_step(&kart,&t,(cnk_driver_input){0,1,0},0.03f,NULL))return 7;
 cnk_kart copy,a,b;
 if(cnk_kart_spawn(&copy,(cnk_vec3){0,0,0},0))return 8;
 a=copy;b=copy;
 for(unsigned i=0;i<400;i++){
  cnk_driver_input in={i<200?1:0,i>=200?0.8f:0,i%100<50?0.9f:-0.9f};
  if(cnk_drive_step(&a,&t,in,1.0f/60.0f,NULL)||
     cnk_drive_step(&b,&t,in,1.0f/60.0f,NULL))return 9;
 }
 if(memcmp(&a,&b,sizeof(a)))return 10;
 /* Ground snapping goes through the exact shared collision-grid API. */
 cnk_geo_triangle terrain={0};
 terrain.v[0].x=-100;terrain.v[0].y=5;terrain.v[0].z=-100;
 terrain.v[1].x=100;terrain.v[1].y=5;terrain.v[1].z=-100;
 terrain.v[2].x=0;terrain.v[2].y=5;terrain.v[2].z=100;
 cnk_geometry g={0};g.triangles=&terrain;g.triangle_count=1;
 cnk_collision_grid grid;
 if(cnk_collision_grid_build(&g,4,4,&grid))return 11;
 cnk_kart grounded;
 if(cnk_kart_spawn(&grounded,(cnk_vec3){0,9,0},0)||
    cnk_drive_step(&grounded,&t,(cnk_driver_input){0,0,0},1.0f/60.0f,&grid)||
    fabsf(grounded.position.y-5.0f)>0.0001f){cnk_collision_grid_free(&grid);return 12;}
 cnk_collision_grid_free(&grid);
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"Usage: test_native_drive data.vfs\n");return 1;}
 int synthetic=test_synthetic();
 if(synthetic){fprintf(stderr,"Synthetic controls failed %d\n",synthetic);return 2;}
 fufs_file f;
 if(fufs_open(argv[1],&f))return 3;
 cnk_drive_tuning tune;cnk_drive_default_tuning(&tune);
 unsigned tracks=0,sections=0,updates=0,finished=0;
 float max_deviation=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t n=0;
  if(fufs_extract(&f,i,&data,&n)){fufs_close(&f);return 4;}
  if(n>=12&&!memcmp(data,"PIFF",4)&&!memcmp(data+8,"SCEN",4)){
   cnk_scene scene;
   if(cnk_parse_scene(data,n,&scene)){free(data);fufs_close(&f);return 5;}
   const cnk_sline *path=cnk_scene_line(&scene,"DRV0");
   cnk_race_track track;
   if(path && path->count>3 && !cnk_race_track_build(&scene,&track)){
    cnk_vec3 p0,p1;
    if(cnk_nav_sample(path,track.gates[0].fraction+0.004f,1,&p0)||
       cnk_nav_sample(path,track.gates[0].fraction+0.006f,1,&p1))return 6;
    float heading=atan2f(p1.x-p0.x,p1.z-p0.z);
    cnk_kart kart;
    cnk_ai_driver ai;
    cnk_race race;
    if(cnk_kart_spawn(&kart,p0,heading)||
       cnk_ai_init(&ai,path,1,&tune)||cnk_race_start(&race,&track,2,0))return 7;
    /* Tests source route guidance, safe input, monotonic positions and
     * interoperability with native checkpoint tracker. Physical constants
     * are preliminary, not asserted to be the exact game physics. */
    for(unsigned step=1;step<=36000;step++){
     cnk_driver_input input;
     if(cnk_ai_controls(&ai,&kart,&tune,&input))return 8;
     if(!isfinite(input.steer)||fabsf(input.steer)>1||
        !isfinite(input.throttle)||!isfinite(input.brake))return 9;
     int result=cnk_drive_race_step(&kart,&tune,input,1.0f/60.0f,NULL,&race,((uint64_t)step*1000u)/60u);
     if(result<0)return 10;
     if(result&CNK_RACE_GATE)sections++;
     if(!isfinite(kart.position.x)||!isfinite(kart.position.z)||
        !isfinite(kart.speed)||fabsf(kart.speed)>tune.max_speed+0.1f)return 11;
     if(ai.route_distance>max_deviation)max_deviation=ai.route_distance;
     updates++;
     if(race.finished)break;
    }
    printf("SCEN %u: native AI two-lap integration: last path deviation=%.2f, completed_laps=%u\n",
           i,ai.route_distance,race.completed_laps);
    tracks++;if(race.finished && race.completed_laps==2)finished++;
   }
   cnk_scene_free(&scene);
  }
  free(data);
 }
 fufs_close(&f);
 printf("NATIVE DRIVING: routes=%u two-lap-completions=%u updates=%u crossed_gates=%u max_deviation=%.1f\n",
        tracks,finished,updates,sections,max_deviation);
 if(tracks!=12||finished!=12||updates<10000||max_deviation>200.0f)return 12;
 puts("PASS: native driving/controller test only. NOT original handling or a finished game.");
 return 0;
}
