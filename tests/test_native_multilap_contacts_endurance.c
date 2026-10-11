/* Multi-lap regression using ORIGINAL Zeebo DRV0/LAP* route geometry.
 * AI driving/grid and kart-to-kart collisions remain EXPERIMENTAL, not
 * historically verified.  The host simulation is not Vita hardware QA. */
#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void run_race(const fufs_file *source, uint32_t scene_id,
                     unsigned laps, unsigned max_frames){
 cnk_game game={0};
 assert(cnk_game_load_original_start(&game,source,scene_id,6,laps)==0);
 assert(cnk_game_enable_provisional_kart_contacts(&game,1)==0);
 uint32_t prev_laps[CNK_MAX_RACERS]={0};
 uint64_t prev_times[CNK_MAX_RACERS]={0};
 unsigned frame=0;
 for(;frame<max_frames;frame++){
  cnk_driver_input human={0};
  if(!game.session.racers[0].race.finished)
   assert(cnk_ai_controls(&game.session.racers[0].ai,
      &game.session.racers[0].kart,&game.session.tuning,&human)==0);
  /* 16667 us at 60 Hz accumulates an extra 20 microframes each tick.
   * Around tick 50000 it legitimately emits TWO physics steps. */
  int steps=cnk_game_update(&game,human,16667);
  assert(steps>=1 && steps<=2);
  unsigned finished=0;
  for(unsigned k=0;k<6;k++){
   const cnk_session_racer *r=&game.session.racers[k];
   assert(isfinite(r->kart.position.x)&&isfinite(r->kart.position.y));
   assert(isfinite(r->kart.position.z)&&isfinite(r->kart.heading));
   assert(r->race.completed_laps>=prev_laps[k]);
   assert(r->race.completed_laps<=laps);
   assert(r->race.last_ms>=prev_times[k]);
   prev_laps[k]=r->race.completed_laps;
   prev_times[k]=r->race.last_ms;
   if(r->race.finished){
    assert(r->race.completed_laps==laps);
    assert(r->finish_ms<=game.session.now_ms);
    finished++;
   }
  }
  if(finished==6)break;
 }
 if(frame==max_frames){
  fprintf(stderr,"STALLED: scene %u, %u laps after %u frames\n",scene_id,laps,frame);
  for(unsigned k=0;k<6;k++){
   cnk_session_racer *r=&game.session.racers[k];
   fprintf(stderr,"  slot %u laps=%u nextgate=%u finished=%u recover=%u\n",
     k,r->race.completed_laps,r->race.next_gate,r->race.finished,r->recovery_count);
  }
 }
 assert(frame<max_frames);
 cnk_game_classification results={0};
 assert(cnk_game_classification_read(&game,&results)==0);
 assert(results.count==6);
 for(unsigned k=0;k<6;k++){
  assert(results.rows[k].rank==k+1);
  assert(results.rows[k].finished);
  assert(results.rows[k].completed_laps==laps);
  if(k)assert(results.rows[k].elapsed_ms>=results.rows[k-1].elapsed_ms);
 }
 printf("PASS: scene %u, %u laps, six finishers, %u simulated frames\n",
        scene_id,laps,frame+1);fflush(stdout);
 cnk_game_unload(&game);
}
int main(int argc,char **argv){
 assert(argc==2 || (argc==3 && (strcmp(argv[2],"--quick")==0 || strcmp(argv[2],"--five-laps")==0)));
 fufs_file source={0}; assert(fufs_open(argv[1],&source)==0);
 if(argc==3 && strcmp(argv[2],"--quick")==0)run_race(&source,37,2,60000);
 else{
  const uint32_t scenes[]={37,39,42,69,73,77,207,211,213,295,298,300};
  const unsigned laps=argc==3 ? 5u : 3u;
  for(unsigned i=0;i<sizeof(scenes)/sizeof(scenes[0]);i++)
   run_race(&source,scenes[i],laps,laps==5u ? 150000u : 90000u);
 }
 fufs_close(&source);
 return 0;
}
