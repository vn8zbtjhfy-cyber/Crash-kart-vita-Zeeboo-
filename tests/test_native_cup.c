#include "cnk_cup.h"
#include "cnk_frontend_actions.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

static cnk_game_classification make_board(uint32_t count,const uint32_t *slots){
 cnk_game_classification b={0};b.count=count;
 for(uint32_t i=0;i<count;i++){
  b.rows[i].racer_slot=slots[i];
  b.rows[i].rank=i+1;
  b.rows[i].completed_laps=2;
  b.rows[i].total_laps=2;
  b.rows[i].finished=1;
  b.rows[i].elapsed_ms=60000+i*10000;
 }
 return b;
}
static void pure_scoring(const cnk_track_catalog *catalog){
 assert(catalog->race_count>=3);
 uint32_t ids[2]={cnk_track_race_at(catalog,0)->resource_id,
                  cnk_track_race_at(catalog,2)->resource_id};
 uint32_t scoring[3]={10,6,3};
 cnk_cup c={0},original={0};
 assert(!cnk_cup_init(&c,catalog,ids,2,scoring,3));
 assert(c.active&&c.round_count==2);
 original=c;
 assert(cnk_cup_init(&c,catalog,ids,0,scoring,3)<0&&!memcmp(&c,&original,sizeof(c)));
 uint32_t dupe[2]={ids[0],ids[0]};
 assert(cnk_cup_init(&c,catalog,dupe,2,scoring,3)<0&&!memcmp(&c,&original,sizeof(c)));
 uint32_t unknown[2]={ids[0],UINT32_MAX};
 assert(cnk_cup_init(&c,catalog,unknown,2,scoring,3)<0&&!memcmp(&c,&original,sizeof(c)));
 uint32_t bad_scores[3]={10,10,3};
 assert(cnk_cup_init(&c,catalog,ids,2,bad_scores,3)<0&&!memcmp(&c,&original,sizeof(c)));
 uint32_t huge[3]={UINT32_MAX,8,3};
 assert(cnk_cup_init(&c,catalog,ids,2,huge,3)<0&&!memcmp(&c,&original,sizeof(c)));
 uint32_t ordinal=UINT32_MAX;
 assert(cnk_cup_next_race(&c,&ordinal)==1&&ordinal==0);
 uint32_t slots1[3]={2,0,1};
 cnk_game_classification b=make_board(3,slots1);
 cnk_game_classification malformed=b;
 malformed.rows[1].racer_slot=2;
 assert(cnk_cup_commit(&c,ids[0],&malformed)<0&&!memcmp(&c,&original,sizeof(c)));
 malformed=b;malformed.rows[2].finished=0;
 assert(cnk_cup_commit(&c,ids[0],&malformed)<0&&!memcmp(&c,&original,sizeof(c)));
 malformed=b;malformed.rows[0].completed_laps=1;
 assert(cnk_cup_commit(&c,ids[0],&malformed)<0&&!memcmp(&c,&original,sizeof(c)));
 assert(cnk_cup_commit(&c,ids[1],&b)<0&&!memcmp(&c,&original,sizeof(c)));
 assert(!cnk_cup_commit(&c,ids[0],&b));
 assert(c.completed_rounds==1&&c.total_points[2]==10&&c.total_points[0]==6&&c.total_points[1]==3);
 cnk_cup prior=c;
 assert(cnk_cup_commit(&c,ids[0],&b)<0&&!memcmp(&c,&prior,sizeof(c)));
 assert(cnk_cup_next_race(&c,&ordinal)==1&&ordinal==2);
 uint32_t slots2[3]={0,1,2};
 cnk_game_classification second=make_board(3,slots2);
 assert(!cnk_cup_commit(&c,ids[1],&second));
 assert(cnk_cup_next_race(&c,&ordinal)==0);
 cnk_cup_standings s={0};assert(!cnk_cup_standings_read(&c,&s));
 assert(s.finished&&s.completed_rounds==2&&s.count==3);
 assert(s.rows[0].slot==0 && s.rows[0].points==16);
 assert(s.rows[1].slot==2 && s.rows[1].points==13);
 assert(s.rows[2].slot==1 && s.rows[2].points==9);
 prior=c;assert(cnk_cup_commit(&c,ids[1],&second)<0&&!memcmp(&c,&prior,sizeof(c)));
}
static void real_rom_rounds(const fufs_file *vfs){
 cnk_frontend ui={0};
 assert(!cnk_frontend_init(&ui,vfs,3,2));
 uint32_t ids[2]={cnk_track_race_at(&ui.catalog,1)->resource_id,
                  cnk_track_race_at(&ui.catalog,3)->resource_id};
 uint32_t points[3]={9,5,2};
 assert(!cnk_frontend_cup_begin(&ui,ids,2,points));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.game.loaded&&ui.selected_race==1);
 assert(cnk_frontend_select(&ui,1)<0);
 for(int i=0;i<185;i++)assert(cnk_frontend_update(&ui,(cnk_driver_input){0},16667)>=0);
 assert(cnk_start_sequence_racing(&ui.start_sequence));
 /* Simulated finish is used ONLY to test the menu state transition.
  * Original track/race loading, actual countdown and rendering inputs are real. */
 cnk_session *ss=&ui.game.session;
 for(uint32_t k=0;k<3;k++){
  ss->racers[k].race.finished=1;
  ss->racers[k].race.completed_laps=2;
  ss->racers[k].finish_ms=ss->now_ms;
 }
 ss->racers[1].race.finished=0;
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.cup.completed_rounds==0);
 ss->racers[1].race.finished=1;
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.state==CNK_FRONTEND_RESULTS&&ui.cup.completed_rounds==1);
 assert(!cnk_frontend_cup_next(&ui));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.selected_race==3);
 assert(ui.game.scene_index==cnk_track_race_at(&ui.catalog,3)->vfs_index);
 assert(ui.cup.completed_rounds==1);
 /* Restarting a Cup round never resets the championship tally. */
 assert(!cnk_frontend_restart(&ui));
 assert(ui.cup.completed_rounds==1&&ui.selected_race==3);
 for(int i=0;i<185;i++)assert(cnk_frontend_update(&ui,(cnk_driver_input){0},16667)>=0);
 ss=&ui.game.session;
 for(uint32_t k=0;k<3;k++){
  ss->racers[k].race.finished=1;
  ss->racers[k].race.completed_laps=2;
  ss->racers[k].finish_ms=ss->now_ms;
 }
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.state==CNK_FRONTEND_RESULTS&&ui.cup.completed_rounds==2);
 assert(cnk_frontend_cup_next(&ui)==1);
 assert(ui.state==CNK_FRONTEND_RESULTS);
 assert(!cnk_frontend_back_to_select(&ui));
 assert(!ui.cup.active&&ui.state==CNK_FRONTEND_SELECT);
 cnk_frontend_close(&ui);
}
static void snapshot_test(const fufs_file *vfs){
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 uint32_t ids[2]={cnk_track_race_at(&app.frontend.catalog,0)->resource_id,
                  cnk_track_race_at(&app.frontend.catalog,1)->resource_id};
 uint32_t points[3]={9,5,2};
 assert(!cnk_frontend_cup_begin(&app.frontend,ids,2,points));
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(snap.cup_active && snap.cup_standings.total_rounds==2 &&
        snap.cup_standings.completed_rounds==0 && !snap.cup_standings.finished);
 cnk_app_runtime_close(&app);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 pure_scoring(&cat);
 real_rom_rounds(&vfs);
 snapshot_test(&vfs);
 fufs_close(&vfs);
 puts("CUP PASS: authenticated source routes, validated scoring, atomic standings, two sequential ROM races, retry and stop, snapshot");
 return 0;
}
