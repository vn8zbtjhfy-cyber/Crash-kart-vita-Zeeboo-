#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(&vfs,&catalog));
 cnk_game game={0};cnk_game_classification result={0};
 assert(cnk_game_classification_read(&game,&result)<0);
 assert(!cnk_game_load_catalog_race(&game,&catalog,0,3,2));
 assert(!cnk_game_classification_read(&game,&result));
 assert(result.count==3);
 for(unsigned i=0;i<3;i++){
  assert(result.rows[i].racer_slot==i && result.rows[i].rank==i+1);
  assert(result.rows[i].total_laps==2 && result.rows[i].elapsed_ms==0);
  assert(!result.rows[i].finished);
 }
 cnk_driver_input accelerate={1,0,0};
 for(int i=0;i<62;i++)assert(cnk_game_update(&game,accelerate,16667)>=0);
 assert(!cnk_game_classification_read(&game,&result));
 assert(result.rows[0].elapsed_ms>0);
 /* A simulated finished competitor is ranked ahead of active competitors.
  * No gameplay data is invented or shipped; this tests the read-only view. */
 game.session.racers[2].race.finished=1;
 game.session.racers[2].finish_ms=game.session.now_ms;
 game.session.racers[2].race.completed_laps=2;
 game.session.standings[0]=2;
 game.session.standings[1]=0;
 game.session.standings[2]=1;
 assert(!cnk_game_classification_read(&game,&result));
 assert(result.rows[0].racer_slot==2 && result.rows[0].rank==1);
 assert(result.rows[0].finished && result.rows[0].completed_laps==2);
 assert(result.rows[0].elapsed_ms==game.session.racers[2].finish_ms);
 assert(result.rows[1].racer_slot==0 && !result.rows[1].finished);
 game.session.standings[2]=0;
 assert(cnk_game_classification_read(&game,&result)<0);
 game.session.standings[2]=1;
 game.session.racers[2].finish_ms=0;
 game.session.racers[2].race.start_ms=100;
 assert(cnk_game_classification_read(&game,&result)<0);
 cnk_game_unload(&game);
 assert(cnk_game_classification_read(&game,&result)<0);
 fufs_close(&vfs);
 puts("PASS: read-only race classification, rankings, exact finish times, unfinished status, corrupt standings rejection");
 return 0;
}
