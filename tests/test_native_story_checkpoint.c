#include "cnk_story_checkpoint.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static cnk_story_manifest manifest(const cnk_track_catalog *cat){
 cnk_story_manifest m={0};m.stage_count=3;
 for(uint32_t i=0;i<3;i++){
  const cnk_track_entry *track=cnk_track_race_at(cat,i);assert(track);
  m.stages[i]=(cnk_story_stage){track->resource_id,i,i+2u,i==0?1u:2u};
 }
 return m;
}
static cnk_game_classification board(uint32_t laps,uint32_t player_rank){
 cnk_game_classification b={0};b.count=3;
 for(uint32_t i=0;i<3;i++){
  uint32_t rank=i==0?player_rank:(player_rank==1?i+1u:(i==1?1u:3u));
  b.rows[i].racer_slot=i;b.rows[i].rank=rank;
  b.rows[i].finished=1;b.rows[i].total_laps=laps;
  b.rows[i].completed_laps=laps;
 }
 return b;
}
static void tamper(const char *path){
 FILE *f=fopen(path,"r+b");assert(f);
 assert(!fseek(f,43,SEEK_SET));int c=fgetc(f);assert(c!=EOF);
 assert(!fseek(f,43,SEEK_SET));assert(fputc(c^0x20,f)!=EOF);
 assert(!fclose(f));
}
static void unlink_all(const char *path){
 char tmp[400];remove(path);
 snprintf(tmp,sizeof(tmp),"%s.bak",path);remove(tmp);
 snprintf(tmp,sizeof(tmp),"%s.tmp",path);remove(tmp);
 snprintf(tmp,sizeof(tmp),"%s.bak.tmp",path);remove(tmp);
}
static void checkpoint_core(const fufs_file *vfs,const char *path){
 unlink_all(path);
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(vfs,&cat));
 cnk_story_manifest m=manifest(&cat);
 cnk_story s={0};assert(!cnk_story_init(&s,&cat,m.stages,3,3,0));
 cnk_story result=s;
 assert(cnk_story_checkpoint_save(&s,path)<0);
 assert(cnk_story_checkpoint_load(&result,path,&m,&cat,3,0)==-2);
 assert(!memcmp(&s,&result,sizeof(s)));
 cnk_game_classification finish=board(2,1);
 assert(cnk_story_complete(&s,m.stages[0].resource_id,&finish)==1);
 assert(!cnk_story_checkpoint_save(&s,path));
 assert(!cnk_story_checkpoint_load(&result,path,&m,&cat,3,0));
 assert(!memcmp(&s,&result,sizeof(s)));
 cnk_story before=result;
 /* Reject different original scene, lap count, ranking rule, or player. */
 cnk_story_manifest altered=m;altered.stages[0].laps++;
 assert(cnk_story_checkpoint_load(&result,path,&altered,&cat,3,0)<0);
 assert(!memcmp(&before,&result,sizeof(result)));
 altered=m;altered.stages[1].best_required_rank=1;
 assert(cnk_story_checkpoint_load(&result,path,&altered,&cat,3,0)<0);
 assert(cnk_story_checkpoint_load(&result,path,&m,&cat,3,1)<0);
 /* An impossible awarded rank must never be written. */
 cnk_story bad=s;bad.achieved_ranks[0]=2;
 assert(cnk_story_checkpoint_save(&bad,path)<0);
 bad=s;bad.achieved_ranks[1]=1;
 assert(cnk_story_checkpoint_save(&bad,path)<0);
 bad=s;bad.stages[1].resource_id=bad.stages[0].resource_id;
 assert(cnk_story_checkpoint_save(&bad,path)<0);
 /* Save stage 2, making stage 1 the last-good .bak snapshot. */
 finish=board(3,2);
 assert(cnk_story_complete(&s,m.stages[1].resource_id,&finish)==1);
 assert(!cnk_story_checkpoint_save(&s,path));
 tamper(path);
 assert(cnk_story_checkpoint_load(&result,path,&m,&cat,3,0)<0);
 assert(!memcmp(&before,&result,sizeof(result)));
 assert(!cnk_story_checkpoint_load_backup(&result,path,&m,&cat,3,0));
 assert(result.completed==1 && result.achieved_ranks[0]==1);
 /* A broken primary cannot replace the valid backup during next save. */
 assert(!cnk_story_checkpoint_save(&s,path));
 assert(!cnk_story_checkpoint_load_backup(&result,path,&m,&cat,3,0));
 assert(result.completed==1);
 FILE *f=fopen(path,"ab");assert(f);assert(fputc(0,f)!=EOF);assert(!fclose(f));
 assert(cnk_story_checkpoint_load(&result,path,&m,&cat,3,0)<0);
 assert(!cnk_story_checkpoint_save(&s,path));
 /* Resuming may only happen with source-validated completed stages. */
 cnk_app_runtime app={0};assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_story_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_story_checkpoint_path(&app,path));
 assert(app.frontend.story.completed==2 && app.frontend.state==CNK_FRONTEND_RACE);
 assert(app.frontend.selected_race==2 && app.frontend.total_laps==4);
 assert(cnk_frontend_story_resume(&app.frontend,&s)<0);
 cnk_app_runtime_close(&app);
 finish=board(4,1);
 assert(cnk_story_complete(&s,m.stages[2].resource_id,&finish)==1);
 assert(!cnk_story_checkpoint_save(&s,path));
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_story_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_story_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_SELECT && !app.frontend.story.active);
 cnk_app_snapshot snap={0};assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(snap.has_completed_story && snap.completed_story_count==3 &&
        snap.completed_story_ranks[0]==1 &&
        snap.completed_story_ranks[1]==2 && snap.completed_story_ranks[2]==1);
 cnk_app_runtime_close(&app);
 /* A different objective may not read an archived completed story. */
 altered=m;altered.stages[2].laps=5;
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_story_configure(&app.frontend,&altered));
 assert(cnk_app_runtime_set_story_checkpoint_path(&app,path)<0);
 assert(!app.story_checkpoint_path[0]);
 cnk_app_runtime_close(&app);
 unlink_all(path);
}
static void automatic_commit(const fufs_file *vfs,const char *path){
 unlink_all(path);
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 cnk_story_manifest m=manifest(&app.frontend.catalog);
 assert(!cnk_frontend_story_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_story_checkpoint_path(&app,path));
 uint64_t clock=1000000u;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 clock+=16667u;
 assert(!cnk_app_runtime_tick(&app,clock,CNK_UI_PAUSE,(cnk_driver_input){0},&snap));
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 for(unsigned i=0;i<190;i++){
  clock+=16667u;
  assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 }
 assert(cnk_start_sequence_racing(&app.frontend.start_sequence));
 cnk_story temp={0};
 assert(cnk_story_checkpoint_load(&temp,path,&m,&app.frontend.catalog,3,0)==-2);
 cnk_session *session=&app.frontend.game.session;
 for(uint32_t i=0;i<3;i++){
  session->racers[i].race.finished=1;
  session->racers[i].race.completed_laps=2;
  session->racers[i].finish_ms=session->now_ms + i*10u;
 }
 session->racers[2].race.finished=0;
 clock+=16667u;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 assert(!app.frontend.story.completed);
 assert(cnk_story_checkpoint_load(&temp,path,&m,&app.frontend.catalog,3,0)==-2);
 session->racers[2].race.finished=1;
 clock+=16667u;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 assert(app.frontend.story.completed==1);
 assert(!app.story_write_error);
 assert(!cnk_story_checkpoint_load(&temp,path,&m,&app.frontend.catalog,3,0));
 assert(temp.completed==1);
 cnk_app_runtime_close(&app);
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_story_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_story_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 assert(app.frontend.selected_race==1);
 assert(app.frontend.story.completed==1);
 cnk_app_runtime_close(&app);
 /* No authentic backup yet: a damaged first stage cannot auto-recover. */
 tamper(path);
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_story_configure(&app.frontend,&m));
 assert(cnk_app_runtime_set_story_checkpoint_path(&app,path)<0);
 assert(!app.story_checkpoint_path[0]);
 assert(app.frontend.state==CNK_FRONTEND_SELECT);
 cnk_app_runtime_close(&app);
 unlink_all(path);
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 checkpoint_core(&vfs,argv[2]);
 automatic_commit(&vfs,argv[2]);
 fufs_close(&vfs);
 puts("STORY CHECKPOINT PASS: stage completion, integrity, backups, source-bound resume and automatic commit");
 return 0;
}
