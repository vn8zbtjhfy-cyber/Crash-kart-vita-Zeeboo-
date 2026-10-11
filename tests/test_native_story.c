#include "cnk_story.h"
#include "cnk_frontend_actions.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static cnk_game_classification board(uint32_t first,uint32_t laps){
 cnk_game_classification b={0};b.count=3;
 for(uint32_t i=0;i<3;i++){
  uint32_t slot=(first+i)%3;
  b.rows[i].racer_slot=slot;
  b.rows[i].rank=i+1;
  b.rows[i].finished=1;
  b.rows[i].total_laps=laps;
  b.rows[i].completed_laps=laps;
  b.rows[i].elapsed_ms=45000u+1000u*i;
 }
 return b;
}
static void unit(const cnk_track_catalog *cat){
 const cnk_track_entry *a=cnk_track_race_at(cat,0),*b=cnk_track_race_at(cat,1);
 assert(a&&b);
 cnk_story_stage goals[2]={{a->resource_id,999u,2u,1u},
                            {b->resource_id,999u,3u,2u}};
 cnk_story s={0},pristine={0};
 assert(!cnk_story_init(&s,cat,goals,2,3,0));
 assert(s.stages[0].race_ordinal==0&&s.stages[1].race_ordinal==1);
 pristine=s;
 cnk_story_stage bad[2]={goals[0],goals[0]};
 assert(cnk_story_init(&s,cat,bad,2,3,0)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 bad[1]=goals[1];bad[1].resource_id=UINT32_MAX;
 assert(cnk_story_init(&s,cat,bad,2,3,0)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 bad[1]=goals[1];bad[1].best_required_rank=4;
 assert(cnk_story_init(&s,cat,bad,2,3,0)<0);
 cnk_story_stage stage={0};
 assert(cnk_story_next(&s,&stage)==1 &&stage.resource_id==goals[0].resource_id);
 cnk_game_classification lose=board(1,2);
 assert(cnk_story_complete(&s,a->resource_id,&lose)==0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 assert(cnk_story_complete(&s,b->resource_id,&lose)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 cnk_game_classification corrupt=board(0,2);
 corrupt.rows[2].racer_slot=1;
 assert(cnk_story_complete(&s,a->resource_id,&corrupt)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 corrupt=board(0,2);corrupt.rows[1].finished=0;
 assert(cnk_story_complete(&s,a->resource_id,&corrupt)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 corrupt=board(0,3);
 assert(cnk_story_complete(&s,a->resource_id,&corrupt)<0 &&
        !memcmp(&s,&pristine,sizeof(s)));
 cnk_game_classification win=board(0,2);
 assert(cnk_story_complete(&s,a->resource_id,&win)==1);
 assert(s.completed==1&&s.achieved_ranks[0]==1);
 cnk_story saved=s;
 assert(cnk_story_complete(&s,a->resource_id,&win)<0 &&
        !memcmp(&s,&saved,sizeof(s)));
 assert(cnk_story_next(&s,&stage)==1 && stage.resource_id==b->resource_id);
 cnk_game_classification second=board(2,3); /* player rank 2 */
 assert(cnk_story_complete(&s,b->resource_id,&second)==1);
 assert(s.completed==2&&s.achieved_ranks[1]==2);
 assert(cnk_story_next(&s,&stage)==0);
 saved=s;
 assert(cnk_story_complete(&s,b->resource_id,&second)<0&&
        !memcmp(&s,&saved,sizeof(s)));
}
static void fake_finish(cnk_frontend *ui,const uint32_t rank_by_slot[3]){
 cnk_session *session=&ui->game.session;
 for(uint32_t j=0;j<3;j++){
  session->racers[j].race.finished=1;
  session->racers[j].race.completed_laps=ui->total_laps;
  session->racers[j].race.total_laps=ui->total_laps;
  session->racers[j].finish_ms=session->now_ms+(uint64_t)rank_by_slot[j]*1000u;
  session->standings[rank_by_slot[j]-1u]=j;
 }
}
static void frontend(const fufs_file *vfs,const cnk_track_catalog *cat){
 cnk_frontend ui={0};
 assert(!cnk_frontend_init(&ui,vfs,3,2));
 cnk_story_stage goals[2]={{cnk_track_race_at(cat,0)->resource_id,0,2,1},
                            {cnk_track_race_at(cat,1)->resource_id,0,3,1}};
 cnk_story_stage bad[2]={goals[0],goals[0]};
 assert(cnk_frontend_story_begin(&ui,bad,2)<0 &&ui.state==CNK_FRONTEND_SELECT);
 assert(!cnk_frontend_story_begin(&ui,goals,2));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.story.active&&ui.selected_race==0);
 assert(cnk_frontend_toggle_practice(&ui)<0);
 assert(cnk_frontend_select(&ui,1)<0);
 assert(ui.total_laps==2);
 for(int i=0;i<185;i++)assert(cnk_frontend_update(&ui,(cnk_driver_input){0},16667)>=0);
 /* Driver player rank 3: stage must be replayed. */
 const uint32_t losing[3]={3,1,2};
 fake_finish(&ui,losing);
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.state==CNK_FRONTEND_RESULTS&&ui.story.completed==0 &&
        !ui.story_last_attempt_success);
 assert(cnk_frontend_story_next(&ui)<0);
 assert(!cnk_frontend_restart(&ui));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.story.completed==0);
 for(int i=0;i<185;i++)assert(cnk_frontend_update(&ui,(cnk_driver_input){0},16667)>=0);
 const uint32_t winning[3]={1,2,3};
 fake_finish(&ui,winning);
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.state==CNK_FRONTEND_RESULTS&&ui.story.completed==1 &&
        ui.story_last_attempt_success);
 assert(!cnk_frontend_story_next(&ui));
 assert(ui.state==CNK_FRONTEND_RACE&&ui.selected_race==1&&ui.total_laps==3);
 assert(ui.story.completed==1&&!ui.story_last_attempt_success);
 for(int i=0;i<185;i++)assert(cnk_frontend_update(&ui,(cnk_driver_input){0},16667)>=0);
 fake_finish(&ui,winning);
 assert(!cnk_frontend_refresh(&ui));
 assert(ui.story.completed==2&&ui.state==CNK_FRONTEND_RESULTS);
 assert(cnk_frontend_story_next(&ui)==1);
 assert(!cnk_frontend_back_to_select(&ui));
 assert(!ui.story.active&&ui.state==CNK_FRONTEND_SELECT);
 cnk_frontend_close(&ui);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 assert(cat.race_count>=2);
 unit(&cat);
 frontend(&vfs,&cat);
 fufs_close(&vfs);
 puts("STORY CORE PASS: verified tracks, objective guard, failed retry, multiple rounds, correct laps, no invented plot");
 return 0;
}
