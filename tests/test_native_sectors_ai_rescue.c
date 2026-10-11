#include "cnk_ai_watchdog.h"
#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

static void test_sectors(void){
 cnk_race_track t={0};t.gate_count=3;
 for(unsigned i=0;i<3;i++){
  t.gates[i].a=(cnk_vec3){10.0f*i,0,-2};
  t.gates[i].b=(cnk_vec3){10.0f*i,0,2};
  t.gates[i].direction=-1;
 }
 cnk_race r={0};
 assert(!cnk_race_start(&r,&t,3,1000));
 assert(r.last_sector_gate==UINT32_MAX && r.sector_valid);
 assert(cnk_race_advance(&r,(cnk_vec3){9,0,0},(cnk_vec3){11,0,0},1100)==CNK_RACE_GATE);
 assert(r.last_sector_ms==50 && r.best_sector_ms[1]==50 && r.last_sector_gate==1);
 assert(cnk_race_advance(&r,(cnk_vec3){19,0,0},(cnk_vec3){21,0,0},1300)==CNK_RACE_GATE);
 assert(r.last_sector_ms==150 && r.best_sector_ms[2]==150);
 assert(cnk_race_advance(&r,(cnk_vec3){-1,0,0},(cnk_vec3){1,0,0},1700)==(CNK_RACE_GATE|CNK_RACE_LAP));
 assert(r.completed_laps==1 && r.best_sector_ms[0]==300 && r.last_sector_ms==300);
 /* Slower sector does not overwrite best. */
 assert(cnk_race_advance(&r,(cnk_vec3){9,0,0},(cnk_vec3){11,0,0},1900)==CNK_RACE_GATE);
 assert(r.best_sector_ms[1]==50 && r.last_sector_ms==300);
 assert(cnk_race_advance(&r,(cnk_vec3){19,0,0},(cnk_vec3){21,0,0},1980)==CNK_RACE_GATE);
 assert(r.best_sector_ms[2]==140);
 /* Out of order: do not credit a sector spanning skipped checkpoint(s). */
 assert(cnk_race_advance(&r,(cnk_vec3){9,0,0},(cnk_vec3){11,0,0},2100)==0);
 assert(!r.sector_valid && r.next_gate==1);
 uint64_t saved=r.best_sector_ms[1];
 assert(cnk_race_advance(&r,(cnk_vec3){9,0,0},(cnk_vec3){11,0,0},2200)==CNK_RACE_GATE);
 assert(r.sector_valid && r.best_sector_ms[1]==saved);
 assert(cnk_race_advance(&r,(cnk_vec3){19,0,0},(cnk_vec3){21,0,0},2290)==CNK_RACE_GATE);
 assert(r.last_sector_ms==95 && r.best_sector_ms[2]==95);
 assert(cnk_race_advance(&r,(cnk_vec3){-1,0,0},(cnk_vec3){1,0,0},2350)==(CNK_RACE_GATE|CNK_RACE_LAP));
 assert(r.completed_laps==2);
}
static void test_watchdog(void){
 cnk_ai_watchdog w={0};cnk_vec3 a={10,1,20};
 assert(!cnk_ai_watchdog_step(&w,a,0.02f,1));
 for(int i=0;i<650;i++)assert(!cnk_ai_watchdog_step(&w,a,1.0f/60,1));
 assert(cnk_ai_watchdog_step(&w,a,1.0f/60,0)==0);
 assert(!w.initialized);
 assert(!cnk_ai_watchdog_step(&w,a,0.02f,1));
 for(int i=0;i<700;i++)assert(!cnk_ai_watchdog_step(&w,a,1.0f/60,1));
 int recovered=0;
 for(int i=0;i<60;i++)if(cnk_ai_watchdog_step(&w,a,1.0f/60,1)==1)recovered++;
 assert(recovered==1);
 assert(!cnk_ai_watchdog_step(&w,(cnk_vec3){14,1,20},0.02f,1));
 assert(w.still_seconds==0);
 assert(cnk_ai_watchdog_step(&w,(cnk_vec3){NAN,0,0},0.02f,1)<0);
 assert(cnk_ai_watchdog_step(&w,a,-1.0f,1)<0);
}
static void test_source_race(const char *vfs_path){
 fufs_file vfs={0};assert(!fufs_open(vfs_path,&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&cat,0,2,2));
 cnk_session *s=&g.session;
 cnk_session_racer *r=&s->racers[1];
 cnk_kart spawn=r->kart;
 uint32_t gate=r->race.next_gate;
 uint64_t start=r->race.start_ms;
 /* Force the opponent into a repeatable no-translation condition, as an
  * impassable wall would: the AI has normal steering but cannot gain ground. */
 for(unsigned frame=0;frame<800 && !r->recovery_count;frame++){
  r->kart=spawn;
  cnk_driver_input no_input={0};
  assert(cnk_session_step(s,no_input,1.0f/60.0f,(uint64_t)(frame+1)*17,NULL)>=0);
 }
 assert(r->recovery_count==1);
 assert(r->race.next_gate==gate && r->race.completed_laps==0);
 assert(r->race.start_ms==start && r->race.last_lap_ms==0);
 assert(r->race.last_accepted_gate==UINT32_MAX);
 assert(!r->ai.progress_initialized);
 assert(r->kart.ticks>0);
 assert(s->racers[0].recovery_count==0);
 assert(isfinite(r->kart.position.x) && isfinite(r->kart.position.z));
 assert(r->ai_watchdog.still_seconds==0);
 assert(cnk_session_position(s,1)>=1);
 cnk_game_hud hud={0};assert(!cnk_game_hud_read(&g,&hud));
 assert(hud.last_sector_gate==UINT32_MAX);
 cnk_game_unload(&g);fufs_close(&vfs);
}
int main(int argc,char **argv){
 assert(argc==2);
 test_sectors();test_watchdog();test_source_race(argv[1]);
 puts("PASS original-route sectors + anti-stall AI rescue (no synthetic lap/clock/finish)");
 return 0;
}
