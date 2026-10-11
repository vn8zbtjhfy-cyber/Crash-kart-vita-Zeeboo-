/* Full development-race regression using six racers, original STRT for the
 * human and original DRV0/LAP* routes from the twelve validated Zeebo scenes.
 * A test-only route driver supplies the human input so every scene can finish
 * without gamepad input. Neither this driver nor the opponent grid are the
 * reverse-engineered Zeebo AI/physics rules. */
#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file source={0};assert(fufs_open(argv[1],&source)==0);
 const unsigned scenes[12]={37,39,42,69,73,77,207,211,213,295,298,300};
 for(unsigned t=0;t<12;t++){
  cnk_game game={0};
  assert(cnk_game_load_original_start(&game,&source,scenes[t],6,1)==0);
  unsigned steps;
  for(steps=0;steps<26000;steps++){
   cnk_driver_input human={0};
   if(!game.session.racers[0].race.finished)
    assert(cnk_ai_controls(&game.session.racers[0].ai,
      &game.session.racers[0].kart,&game.session.tuning,&human)==0);
   assert(cnk_game_update(&game,human,16667)==1);
   int all_finished=1;
   for(unsigned k=0;k<6;k++){
    const cnk_session_racer *r=&game.session.racers[k];
    assert(isfinite(r->kart.position.x) && isfinite(r->kart.position.y));
    assert(isfinite(r->kart.position.z) && isfinite(r->kart.speed));
    if(!r->race.finished)all_finished=0;
   }
   if(all_finished)break;
  }
  assert(steps<26000);
  for(unsigned k=0;k<6;k++){
   const cnk_session_racer *r=&game.session.racers[k];
   assert(r->race.finished && r->race.completed_laps==1);
  }
  cnk_game_classification standings={0};
  assert(cnk_game_classification_read(&game,&standings)==0);
  assert(standings.count==6);
  for(unsigned i=0;i<6;i++){
   assert(standings.rows[i].finished);
   assert(standings.rows[i].completed_laps==1);
   assert(standings.rows[i].rank==i+1);
  }
  printf("SOURCE SCENE %u: 6 finished, %u simulation frames\n",scenes[t],steps+1);
  cnk_game_unload(&game);
 }
 fufs_close(&source);
 puts("PASS: all six racers completed one lap in each of 12 source tracks");
 return 0;
}
