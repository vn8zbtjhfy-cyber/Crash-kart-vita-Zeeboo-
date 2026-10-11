#include "cnk_game.h"
#include "cnk_race.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <stdio.h>

static void test_two_gates_in_one_frame(void){
 cnk_race_track track={0};track.gate_count=3;
 for(int i=0;i<3;i++){
  track.gates[i].a=(cnk_vec3){(float)i*10,0,-2};
  track.gates[i].b=(cnk_vec3){(float)i*10,0,2};
  track.gates[i].direction=-1;
 }
 cnk_race r={0};assert(!cnk_race_start(&r,&track,1,1000));
 int events=cnk_race_advance(&r,(cnk_vec3){5,0,0},(cnk_vec3){25,0,0},1020);
 assert(events==CNK_RACE_GATE);
 assert(r.best_sector_ms[1]==5);
 assert(r.last_sector_gate==2 && r.last_sector_ms==10);
 /* The split from gate 1 to gate 2 is 10 ms, not zero. */
 assert(r.best_sector_ms[2]==10 && r.next_gate==0);
}
static void test_same_frame_finish(void){
 cnk_race_track track={0};track.gate_count=2;
 for(int i=0;i<2;i++){
  track.gates[i].a=(cnk_vec3){(float)i*10,0,-2};
  track.gates[i].b=(cnk_vec3){(float)i*10,0,2};
  track.gates[i].direction=-1;
 }
 cnk_race early={0},late={0};
 assert(!cnk_race_start(&early,&track,1,1000));
 assert(!cnk_race_start(&late,&track,1,1000));
 early.next_gate=late.next_gate=0;
 assert(!cnk_race_advance(&early,(cnk_vec3){-2,0,0},(cnk_vec3){-2,0,0},1100));
 assert(!cnk_race_advance(&late,(cnk_vec3){-8,0,0},(cnk_vec3){-8,0,0},1100));
 int a=cnk_race_advance(&early,(cnk_vec3){-2,0,0},(cnk_vec3){8,0,0},1120);
 int b=cnk_race_advance(&late,(cnk_vec3){-8,0,0},(cnk_vec3){2,0,0},1120);
 assert((a&CNK_RACE_FINISH)&&(b&CNK_RACE_FINISH));
 assert(early.finish_ms==1104 && late.finish_ms==1116);
 assert(early.last_lap_ms==104 && late.last_lap_ms==116);
 assert(early.finish_ms<late.finish_ms);
}
static void test_original_source_rank(const char *path){
 fufs_file vfs={0};assert(!fufs_open(path,&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&cat,0,2,1));
 for(unsigned i=0;i<2;i++){
  cnk_session_racer *r=&g.session.racers[i];
  r->race.finished=1;r->race.running=0;r->race.completed_laps=1;
  r->race.finish_ms=1100+i*10;
  r->finish_ms=1100+i*10;
 }
 /* Slot 0 won despite both finishes occurring in the same 60 Hz tick. */
 cnk_driver_input idle={0};
 assert(cnk_session_step(&g.session,idle,1.0f/60.0f,1120,NULL)>=0);
 cnk_game_classification results={0};
 assert(!cnk_game_classification_read(&g,&results));
 assert(results.rows[0].racer_slot==0 && results.rows[0].elapsed_ms==1100);
 assert(results.rows[1].racer_slot==1 && results.rows[1].elapsed_ms==1110);
 cnk_game_unload(&g);fufs_close(&vfs);
}
int main(int argc,char **argv){
 assert(argc==2);
 test_two_gates_in_one_frame();
 test_same_frame_finish();
 test_original_source_rank(argv[1]);
 puts("PASS: interpolated checkpoint/sector/finish times and same-frame results from original source race");
 return 0;
}
