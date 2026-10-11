#include "cnk_session.h"
#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int main(int argc,char **argv){
 if(argc!=2)return 1;
 fufs_file f={0};if(fufs_open(argv[1],&f))return 2;
 uint32_t tested=0,finished_player=0,finished_opponents=0,rejected=0;
 for(uint32_t j=0;j<f.count;j++){
  unsigned char *bytes=NULL;size_t n=0;
  if(fufs_extract(&f,j,&bytes,&n))return 3;
  if(n<12||memcmp(bytes+8,"SCEN",4)){free(bytes);continue;}
  cnk_scene sc={0};if(cnk_parse_scene(bytes,n,&sc))return 4;
  free(bytes);
  cnk_race_track candidate={0};int supported=cnk_race_track_build(&sc,&candidate)==0;
  cnk_session s={0};int rc=cnk_session_start(&s,&sc,2,3,0);
  if(!supported){if(rc==0)return 5;rejected++;cnk_scene_free(&sc);continue;}
  if(rc){fprintf(stderr,"scene %u init error %d\n",j,rc);return 6;}
  if(cnk_session_position(&s,0)<1||cnk_session_position(&s,0)>3)return 7;
  if(cnk_session_step(&s,(cnk_driver_input){1,0,0},0.2f,100,NULL)!=-1)return 8;
  if(cnk_session_step(&s,(cnk_driver_input){NAN,0,0},0.016f,100,NULL)!=-1)return 9;
  for(unsigned step=1;step<=36000;step++){
   cnk_driver_input player={0};
   if(!s.racers[0].race.finished &&
      cnk_ai_controls(&s.racers[0].ai,&s.racers[0].kart,&s.tuning,&player))return 10;
   uint64_t clock=((uint64_t)step*1000u)/60u;
   rc=cnk_session_step(&s,player,1.0f/60.0f,clock,NULL);
   if(rc<0){fprintf(stderr,"scene %u step %u, error=%d\n",j,step,rc);return 11;}
   int seen[CNK_MAX_RACERS]={0};
   for(int k=0;k<3;k++){
    int rank=cnk_session_position(&s,(uint32_t)k);
    if(rank<1||rank>3||seen[rank]++)return 12;
    cnk_kart *kart=&s.racers[k].kart;
    if(!isfinite(kart->position.x)||!isfinite(kart->position.y)||
       !isfinite(kart->position.z)||fabsf(kart->speed)>s.tuning.max_speed+0.1f)return 13;
   }
   if(!isfinite(s.camera.eye.x)||!isfinite(s.camera.eye.z))return 14;
   if(s.racers[0].race.finished&&s.racers[1].race.finished&&s.racers[2].race.finished)break;
  }
  uint32_t nfinished=0;
  for(int k=0;k<3;k++)nfinished+=s.racers[k].race.finished?1u:0u;
  printf("scene %u session: %u/3 finished, player laps=%u, rank=%d\n",j,
         nfinished,s.racers[0].race.completed_laps,cnk_session_position(&s,0));
  if(s.racers[0].race.finished)finished_player++;
  if(s.racers[1].race.finished&&s.racers[2].race.finished)finished_opponents++;
  tested++;cnk_scene_free(&sc);
 }
 fufs_close(&f);
 printf("SESSION: scenes=%u, player finished=%u, 2 AI finished=%u, rejected scenes=%u\n",
 tested,finished_player,finished_opponents,rejected);
 if(tested!=12||rejected!=2||finished_player!=12||finished_opponents!=12)return 15;
 puts("PASS: original source routes coordinate player, rivals, checkpoint races, clock, standings, camera.");
 return 0;
}
