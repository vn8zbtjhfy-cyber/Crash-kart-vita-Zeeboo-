/* Advance 96: real source-backed final table, validated previous-round backup
 * and corruption rejection. Does not assert any official Zeebo Cup rules. */
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static cnk_game_classification result(uint32_t n,const uint32_t *ranks){
 cnk_game_classification b={0};b.count=n;
 for(uint32_t i=0;i<n;i++){
  b.rows[i].racer_slot=i;b.rows[i].rank=ranks[i];
  b.rows[i].finished=1;b.rows[i].total_laps=b.rows[i].completed_laps=2;
 }
 return b;
}
static void flip(const char *path){
 FILE *f=fopen(path,"r+b");assert(f);
 assert(!fseek(f,52,SEEK_SET));
 int c=fgetc(f);assert(c!=EOF);
 assert(!fseek(f,52,SEEK_SET));
 assert(fputc(c^0x20,f)!=EOF);assert(!fclose(f));
}
static void clean(const char *path){
 char bak[256],temp[256],baktemp[256];
 assert(strlen(path)<240u);
 snprintf(bak,sizeof(bak),"%s.bak",path);
 snprintf(temp,sizeof(temp),"%s.tmp",path);
 snprintf(baktemp,sizeof(baktemp),"%s.bak.tmp",path);
 remove(path);remove(bak);remove(temp);remove(baktemp);
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file source={0};assert(!fufs_open(argv[1],&source));
 const char *path=argv[2];clean(path);
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(&source,&catalog));
 assert(catalog.race_count>=3);
 cnk_cup_manifest manifest={0};manifest.round_count=3;manifest.racer_count=3;
 for(uint32_t i=0;i<3;i++){
  manifest.resource_ids[i]=cnk_track_race_at(&catalog,i)->resource_id;
 }
 manifest.points[0]=10;manifest.points[1]=6;manifest.points[2]=3;
 cnk_cup cup={0};
 assert(!cnk_cup_init(&cup,&catalog,manifest.resource_ids,3,manifest.points,3));
 const uint32_t r0[]={3,1,2},r1[]={2,3,1},r2[]={1,2,3};
 cnk_game_classification b=result(3,r0);
 assert(!cnk_cup_commit(&cup,manifest.resource_ids[0],&b));
 assert(!cnk_cup_checkpoint_save(&cup,path));
 /* Round two updates the primary while preserving last good round one. */
 b=result(3,r1);
 assert(!cnk_cup_commit(&cup,manifest.resource_ids[1],&b));
 assert(!cnk_cup_checkpoint_save(&cup,path));
 cnk_cup backup={0};
 assert(!cnk_cup_checkpoint_load_backup(&backup,path,&manifest,&catalog));
 assert(backup.completed_rounds==1 && backup.round_ranks[0][1]==1);
 assert(backup.total_points[1]==10);
 flip(path);
 assert(cnk_cup_checkpoint_load(&(cnk_cup){0},path,&manifest,&catalog)<0);
 cnk_app_runtime app={0};
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&manifest));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 assert(app.frontend.cup.completed_rounds==1);
 assert(app.frontend.selected_race==1);
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 /* Loading backup does not silently rewrite a corrupted primary. */
 assert(cnk_cup_checkpoint_load(&(cnk_cup){0},path,&manifest,&catalog)<0);
 cnk_app_runtime_close(&app);
 /* A backup from another Cup must never be accepted. */
 cnk_cup_manifest wrong=manifest;wrong.points[0]=11;
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&wrong));
 assert(cnk_app_runtime_set_cup_checkpoint_path(&app,path)<0);
 assert(app.frontend.state==CNK_FRONTEND_SELECT);
 cnk_app_runtime_close(&app);
 char bakpath[256];snprintf(bakpath,sizeof(bakpath),"%s.bak",path);
 flip(bakpath);
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&manifest));
 assert(cnk_app_runtime_set_cup_checkpoint_path(&app,path)<0);
 assert(!app.cup_checkpoint_path[0]);
 cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(!snap.has_completed_cup);
 cnk_app_runtime_close(&app);
 /* Completed championship: archival classification shown without trying
  * to replay a finished Cup on restart. */
 b=result(3,r2);
 assert(!cnk_cup_commit(&cup,manifest.resource_ids[2],&b));
 assert(!cnk_cup_checkpoint_save(&cup,path));
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!cnk_frontend_cup_configure(&app.frontend,&manifest));
 assert(!cnk_app_runtime_set_cup_checkpoint_path(&app,path));
 assert(app.frontend.state==CNK_FRONTEND_SELECT);
 assert(!app.frontend.cup.active);
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(snap.has_completed_cup && snap.completed_cup_standings.finished);
 cnk_cup_standings expected={0};assert(!cnk_cup_standings_read(&cup,&expected));
 assert(!memcmp(&expected,&snap.completed_cup_standings,sizeof(expected)));
 /* Fresh opt-in Cup begins at zero points; archived final scores are separate. */
 assert(!cnk_frontend_cup_begin(&app.frontend,manifest.resource_ids,
                                 manifest.round_count,manifest.points));
 assert(app.frontend.cup.completed_rounds==0);
 for(uint32_t i=0;i<3;i++)assert(!app.frontend.cup.total_points[i]);
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(snap.has_completed_cup && snap.cup_active);
 assert(snap.cup_standings.completed_rounds==0);
 cnk_app_runtime_close(&app);
 clean(path);fufs_close(&source);
 puts("CUP ARCHIVE PASS: verified final standings, previous-round fallback, corruption rejection, fresh Cup isolation");
 return 0;
}
