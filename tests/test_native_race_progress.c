#include "cnk_race_progress.h"
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static float wrapped(float x){return x-floorf(x);}
static void unit_boundaries(void){
 cnk_race_track track={0};track.gate_count=4;
 track.gates[0].fraction=.80f;
 track.gates[1].fraction=.05f;
 track.gates[2].fraction=.30f;
 track.gates[3].fraction=.60f;
 cnk_race state={0};
 cnk_race_progress p={0};
 assert(!cnk_race_start(&state,&track,3,0));
 assert(!cnk_race_progress_read(&state,.925f,&p));
 assert(p.passed_gates==0 && fabsf(p.sector_fraction-.5f)<.0001f);
 /* A missed gate is not awarded merely because a kart reaches sector 3. */
 assert(!cnk_race_progress_read(&state,.65f,&p));
 assert(p.passed_gates==0 && p.sector_fraction==0);
 state.next_gate=2;
 assert(!cnk_race_progress_read(&state,.175f,&p));
 assert(p.passed_gates==1 && fabsf(p.sector_fraction-.5f)<.0001f);
 state.next_gate=0;
 assert(!cnk_race_progress_read(&state,.70f,&p));
 assert(p.passed_gates==3 && fabsf(p.sector_fraction-.5f)<.0001f);
 /* Reaching a completed lap is a separate validated finish event; this
  * estimator must never advance the lap count or next checkpoint. */
 assert(state.completed_laps==0 && state.next_gate==0);
 assert(cnk_race_progress_read(&state,NAN,&p)<0);
 assert(cnk_race_progress_read(&state,INFINITY,&p)<0);
 assert(cnk_race_progress_read(&state,-.01f,&p)<0);
 assert(cnk_race_progress_read(&state,1.01f,&p)<0);
 state.next_gate=4;
 assert(cnk_race_progress_read(&state,.3f,&p)<0);
 state.next_gate=1;
 track.gates[1].fraction=track.gates[0].fraction;
 assert(cnk_race_progress_read(&state,.90f,&p)<0);
}

static void real_source_standings(const char *rom){
 fufs_file vfs={0};assert(!fufs_open(rom,&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 cnk_game game={0};
 assert(!cnk_game_load_catalog_race(&game,&cat,0,2,2));
 cnk_session *s=&game.session;
 assert(s->track.gate_count>=3);
 float g0=s->track.gates[0].fraction;
 float g1=s->track.gates[1].fraction;
 float g2=s->track.gates[2].fraction;
 float a=wrapped(g2+.005f);   /* shortcut: far ahead, first checkpoint not passed */
 float b=wrapped(g1+.005f);   /* legitimate: gate 1 actually passed */
 cnk_vec3 pa,pb;
 assert(!cnk_nav_sample(s->route,a,1,&pa));
 assert(!cnk_nav_sample(s->route,b,1,&pb));
 assert(wrapped(a-g0)>wrapped(b-g0));
 s->racers[0].race.next_gate=1;
 s->racers[1].race.next_gate=2;
 s->racers[0].kart.position=pa;s->racers[0].kart.speed=0;
 s->racers[1].kart.position=pb;s->racers[1].kart.speed=0;
 cnk_driver_input idle={0};
 assert(cnk_session_step(s,idle,1.0f/60.0f,s->now_ms+17,&game.collision)>=0);
 assert(s->racers[0].race.next_gate==1);
 assert(s->racers[1].race.next_gate==2);
 assert(cnk_session_position(s,1)==1);
 assert(cnk_session_position(s,0)==2);
 cnk_game_classification cls={0};
 assert(!cnk_game_classification_read(&game,&cls));
 assert(cls.rows[0].racer_slot==1);
 /* After validating checkpoint 1 the first racer can compete within
  * the same sector, but still cannot be promoted by arbitrary lap count. */
 s->racers[0].race.next_gate=2;
 assert(cnk_session_step(s,idle,1.0f/60.0f,s->now_ms+17,&game.collision)>=0);
 assert(s->racers[0].race.next_gate==2);
 /* Both racers still have zero completed laps, independently of position. */
 assert(!s->racers[0].race.completed_laps&&!s->racers[1].race.completed_laps);
 cnk_game_unload(&game);
 fufs_close(&vfs);
}
int main(int argc,char **argv){
 assert(argc==2);
 unit_boundaries();
 real_source_standings(argv[1]);
 puts("PASS: source checkpoint-authorized race ordering, wrap-around, no missed-gate rank boost, read-only progress");
 return 0;
}
