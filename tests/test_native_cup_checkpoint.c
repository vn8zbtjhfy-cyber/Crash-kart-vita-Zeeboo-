#include "cnk_cup_checkpoint.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cnk_game_classification board(uint32_t n,const uint32_t *ranks){
 cnk_game_classification b={0};b.count=n;
 for(uint32_t i=0;i<n;i++){
  b.rows[i].racer_slot=i;b.rows[i].rank=ranks[i];
  b.rows[i].finished=1;b.rows[i].total_laps=b.rows[i].completed_laps=2;
 }
 return b;
}
static void corrupt(const char *path,long position){
 FILE *f=fopen(path,"r+b");assert(f);
 assert(!fseek(f,position,SEEK_SET));
 int ch=fgetc(f);assert(ch!=EOF);
 assert(!fseek(f,position,SEEK_SET));
 assert(fputc(ch^0x40,f)!=EOF);
 assert(!fclose(f));
}
static void test_checkpoint(const fufs_file *vfs,const char *path){
 (void)remove(path); /* ensure test starts without a stale checkpoint */
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(vfs,&catalog));
 assert(catalog.race_count>=3);
 cnk_cup_manifest m={0};m.round_count=3;m.racer_count=3;
 for(uint32_t i=0;i<3;i++)m.resource_ids[i]=cnk_track_race_at(&catalog,i)->resource_id;
 m.points[0]=10;m.points[1]=6;m.points[2]=3;
 cnk_cup cup={0};assert(!cnk_cup_init(&cup,&catalog,m.resource_ids,3,m.points,3));
 cnk_cup original=cup;
 assert(cnk_cup_checkpoint_save(&cup,path)<0);
 cnk_cup untouched=cup;
 assert(cnk_cup_checkpoint_load(&untouched,path,&m,&catalog)==-2);
 assert(!memcmp(&untouched,&cup,sizeof(cup)));
 uint32_t ranks0[]={2,3,1};
 cnk_game_classification b=board(3,ranks0);
 assert(!cnk_cup_commit(&cup,m.resource_ids[0],&b));
 assert(cup.round_ranks[0][2]==1&&cup.round_ranks[0][0]==2);
 assert(!cnk_cup_checkpoint_save(&cup,path));
 cnk_cup roundtrip={0};assert(!cnk_cup_checkpoint_load(&roundtrip,path,&m,&catalog));
 assert(!memcmp(&cup,&roundtrip,sizeof(cup)));
 /* Missing/different track or scoring configuration is not accepted. */
 cnk_cup_manifest wrong=m;wrong.points[0]=11;
 assert(cnk_cup_checkpoint_load(&roundtrip,path,&wrong,&catalog)<0);
 assert(!memcmp(&cup,&roundtrip,sizeof(cup)));
 wrong=m;wrong.resource_ids[0]=m.resource_ids[1];
 assert(cnk_cup_checkpoint_load(&roundtrip,path,&wrong,&catalog)<0);
 assert(!memcmp(&cup,&roundtrip,sizeof(cup)));
 /* A forged/invalid state cannot be written even with valid Cup IDs. */
 cnk_cup invalid=cup;invalid.total_points[0]++;
 assert(cnk_cup_checkpoint_save(&invalid,path)<0);
 invalid=cup;invalid.round_ranks[0][0]=1;
 assert(cnk_cup_checkpoint_save(&invalid,path)<0);
 invalid=cup;invalid.round_ranks[1][0]=2;
 assert(cnk_cup_checkpoint_save(&invalid,path)<0);
 /* A truncated or corrupt primary never partially mutates *out. */
 corrupt(path,70);
 assert(cnk_cup_checkpoint_load(&roundtrip,path,&m,&catalog)<0);
 assert(!memcmp(&cup,&roundtrip,sizeof(cup)));
 assert(!cnk_cup_checkpoint_save(&cup,path));
 FILE *f=fopen(path,"ab");assert(f);assert(fputc(0,f)!=EOF);assert(!fclose(f));
 assert(cnk_cup_checkpoint_load(&roundtrip,path,&m,&catalog)<0);
 assert(!cnk_cup_checkpoint_save(&cup,path));
 /* The frontend can resume at the next real scene without fabricating laps. */
 cnk_app_runtime app={0};assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_RACE&&app.frontend.cup.completed_rounds==1);
 assert(app.frontend.selected_race==1 && app.frontend.game.loaded);
 assert(app.frontend.cup.total_points[2]==10);
 assert(cnk_frontend_cup_resume(&app.frontend,&cup)<0);
 cnk_app_runtime_close(&app);
 /* Completing every configured round is safe to save and reload as final
  * standings, but must NOT launch an already completed race on startup. */
 uint32_t ranks1[]={1,2,3};b=board(3,ranks1);
 assert(!cnk_cup_commit(&cup,m.resource_ids[1],&b));
 uint32_t ranks2[]={3,1,2};b=board(3,ranks2);
 assert(!cnk_cup_commit(&cup,m.resource_ids[2],&b));
 assert(!cnk_cup_checkpoint_save(&cup,path));
 assert(!cnk_cup_checkpoint_load(&roundtrip,path,&m,&catalog));
 assert(roundtrip.completed_rounds==roundtrip.round_count);
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_SELECT&&!app.frontend.cup.active);
 cnk_app_snapshot finished_snap={0};
 assert(!cnk_app_runtime_snapshot(&app,&finished_snap));
 assert(finished_snap.has_completed_cup &&
        finished_snap.completed_cup_standings.finished);
 assert(finished_snap.completed_cup_standings.completed_rounds==3);
 cnk_cup_standings expected_standings={0};
 assert(!cnk_cup_standings_read(&cup,&expected_standings));
 assert(!memcmp(&finished_snap.completed_cup_standings,
                &expected_standings,sizeof(expected_standings)));
 cnk_app_runtime_close(&app);
 assert(!remove(path));
 (void)original;
}
/* Complete a real source-backed Cup round through the application loop,
 * then reopen the app: the checkpoint must resume the NEXT actual circuit.
 * No simulated completion can be persisted before ALL rivals have finished. */
static void test_automatic_commit(const fufs_file *vfs,const char *path){
 (void)remove(path);
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 cnk_cup_manifest m={0};m.round_count=2;m.racer_count=3;
 m.resource_ids[0]=cnk_track_race_at(&app.frontend.catalog,0)->resource_id;
 m.resource_ids[1]=cnk_track_race_at(&app.frontend.catalog,1)->resource_id;
 m.points[0]=9;m.points[1]=6;m.points[2]=3;
 assert(!cnk_frontend_cup_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 uint64_t clock=1000000;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 clock+=16667;
 assert(!cnk_app_runtime_tick(&app,clock,CNK_UI_PAUSE,(cnk_driver_input){0},&snap));
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 for(unsigned i=0;i<190;i++){
  clock+=16667;
  assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 }
 assert(cnk_start_sequence_racing(&app.frontend.start_sequence));
 assert(cnk_cup_checkpoint_load(&(cnk_cup){0},path,&m,&app.frontend.catalog)==-2);
 cnk_session *ss=&app.frontend.game.session;
 for(uint32_t i=0;i<3;i++){
  ss->racers[i].race.finished=1;
  ss->racers[i].race.completed_laps=2;
  ss->racers[i].finish_ms=ss->now_ms + i*10u;
 }
 ss->racers[2].race.finished=0;
 clock+=16667;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 assert(app.frontend.cup.completed_rounds==0);
 assert(cnk_cup_checkpoint_load(&(cnk_cup){0},path,&m,&app.frontend.catalog)==-2);
 ss->racers[2].race.finished=1;
 clock+=16667;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 assert(app.frontend.state==CNK_FRONTEND_RESULTS);
 assert(app.frontend.cup.completed_rounds==1);
 cnk_cup checkpoint={0};
 assert(!cnk_cup_checkpoint_load(&checkpoint,path,&m,&app.frontend.catalog));
 assert(!app.cup_write_error);
 cnk_app_runtime_close(&app);
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&m));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 assert(app.frontend.selected_race==1);
 assert(app.frontend.cup.completed_rounds==1);
 cnk_app_runtime_close(&app);
 /* Corrupt saves cannot be overwritten by a background load. */
 corrupt(path,52);
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&m));
 assert(cnk_app_runtime_set_cup_checkpoint_path(&app,path)<0);
 assert(!app.cup_checkpoint_path[0]);
 assert(app.frontend.state==CNK_FRONTEND_SELECT);
 cnk_app_snapshot no_archive={0};
 assert(!cnk_app_runtime_snapshot(&app,&no_archive));
 assert(!no_archive.has_completed_cup);
 cnk_app_runtime_close(&app);
 assert(!remove(path));
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 test_checkpoint(&vfs,argv[2]);
 test_automatic_commit(&vfs,argv[2]);
 fufs_close(&vfs);
 puts("CUP CHECKPOINT PASS: source-bound saves, round replay, CRC, atomic failures and authenticated resume");
 return 0;
}
