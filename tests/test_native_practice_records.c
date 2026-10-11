#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

static void tick(cnk_app_runtime *a,uint64_t t,uint32_t keys,cnk_app_snapshot *s){
 assert(!cnk_app_runtime_tick(a,t,keys,(cnk_driver_input){0,0,0},s));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 char path[200];snprintf(path,sizeof(path),"/tmp/cnk56_record_%lu.dat",(unsigned long)getpid());
 remove(path);
 cnk_records records={0},loaded={0};cnk_record found={0};
 assert(!cnk_records_update(&records,123,3,CNK_RECORDS_NORMAL,0,0));
 assert(cnk_records_update(&records,123,3,CNK_RECORDS_NORMAL,50000,130000)==1);
 assert(!cnk_records_update(&records,123,3,CNK_RECORDS_NORMAL,70000,160000));
 assert(cnk_records_update(&records,123,3,CNK_RECORDS_NORMAL,45000,0)==1);
 assert(cnk_records_update(&records,123,3,CNK_RECORDS_PRACTICE,49000,0)==1);
 assert(cnk_records_find(&records,123,3,CNK_RECORDS_NORMAL,&found)==1);
 assert(found.best_lap_ms==45000&&found.best_race_ms==130000);
 assert(cnk_records_find(&records,123,3,CNK_RECORDS_PRACTICE,&found)==1);
 assert(found.best_lap_ms==49000&&found.best_race_ms==0);
 assert(!cnk_records_save(&records,path));
 assert(!cnk_records_load(&loaded,path));
 assert(loaded.count==2);
 FILE *f=fopen(path,"r+b");assert(f);
 assert(!fseek(f,36,SEEK_SET));assert(fputc(0xff,f)!=EOF);fclose(f);
 assert(cnk_records_load(&loaded,path)<0&&loaded.count==2); /* atomic read */
 assert(!cnk_records_save(&records,path));
 f=fopen(path,"ab");assert(f);assert(fputc(0x01,f)!=EOF);fclose(f);
 assert(cnk_records_load(&loaded,path)<0); /* no trailing garbage */
 assert(!cnk_records_save(&records,path));
 f=fopen(path,"r+b");assert(f);
 assert(!fseek(f,12,SEEK_SET));assert(fputc(0,f)!=EOF);fclose(f);
 assert(cnk_records_load(&loaded,path)<0); /* tampered header count */
 assert(!cnk_records_save(&records,path));
 cnk_records full={0};
 for(unsigned k=1;k<=CNK_RECORDS_LIMIT;k++)
  assert(cnk_records_update(&full,k,2,CNK_RECORDS_NORMAL,k*1000,0)==1);
 assert(cnk_records_update(&full,999,2,CNK_RECORDS_NORMAL,999,0)==-2);
 assert(full.count==CNK_RECORDS_LIMIT);
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!cnk_app_runtime_set_records_path(&app,path));
 tick(&app,1000000,0,&snap);
 assert(snap.available_races==12&&snap.state==CNK_FRONTEND_SELECT&&!snap.practice_mode);
 tick(&app,1010000,CNK_UI_RESTART,&snap); /* triangle selects SOLO mode */
 assert(snap.practice_mode&&(snap.app_events&CNK_APP_EVENT_MODE_CHANGED));
 tick(&app,1020000,CNK_UI_RESTART,&snap);
 assert(snap.practice_mode); /* hold edge */
 tick(&app,1030000,0,&snap);
 tick(&app,1040000,CNK_UI_CONFIRM,&snap);
 assert(snap.practice_mode&&snap.classification.count==1);
 assert(app.frontend.game.session.racer_count==1);
 assert(app.frontend.game.source_spawn_enabled);
 assert(app.frontend.game.session.racers[0].race.best_lap_ms==0);
 uint32_t source=snap.source_resource_id;
 /* Start signal must not generate records, even with held throttle. */
 for(unsigned i=0;i<8;i++)tick(&app,1040000+250000u*(i+1),0,&snap);
 assert(!snap.has_personal_best);
 /* Simulate already validated race split, without fabricating a ROM gate.
  * The real race engine is covered by the race / sector tests. */
 app.frontend.start_sequence.phase=CNK_START_RUNNING;
 app.frontend.game.session.racers[0].race.best_lap_ms=44000;
 tick(&app,3210000,0,&snap);
 assert(snap.app_events&CNK_APP_EVENT_NEW_RECORD);
 assert(snap.has_personal_best && snap.personal_best.best_lap_ms==44000);
 assert(!snap.personal_best.best_race_ms);
 assert(!cnk_records_load(&loaded,path));
 assert(cnk_records_find(&loaded,source,2,CNK_RECORDS_PRACTICE,&found)==1);
 assert(found.best_lap_ms==44000);
 tick(&app,3220000,0,&snap);
 assert(!(snap.app_events&CNK_APP_EVENT_NEW_RECORD));
 /* Finish classification and race record persist exactly once. */
 /* An unwritable destination must never abort the gameplay. */
 strcpy(app.records_path,"/tmp/cnk56_missing_dir_surely/records.dat");
 cnk_session_racer *human=&app.frontend.game.session.racers[0];
 human->race.finished=1;
 human->finish_ms=app.frontend.game.session.now_ms;
 tick(&app,3240000,0,&snap);
 assert(snap.state==CNK_FRONTEND_RESULTS);
 assert(snap.app_events&CNK_APP_EVENT_NEW_RECORD);
 assert(snap.personal_best.best_race_ms>0);
 assert(snap.records_write_error!=0); /* no fatal game error */
 strcpy(app.records_path,path);
 assert(!cnk_records_save(&app.records,path));
 uint64_t fastest=snap.personal_best.best_race_ms;
 tick(&app,3250000,0,&snap);
 assert(!(snap.app_events&CNK_APP_EVENT_NEW_RECORD));
 /* Returning to menu preserves the SOLO setting; triangle turns it off. */
 tick(&app,3260000,CNK_UI_CANCEL,&snap);
 assert(snap.state==CNK_FRONTEND_SELECT&&snap.practice_mode);
 tick(&app,3270000,0,&snap);
 tick(&app,3280000,CNK_UI_RESTART,&snap);
 assert(!snap.practice_mode);
 tick(&app,3290000,0,&snap);
 tick(&app,3300000,CNK_UI_CONFIRM,&snap);
 assert(!snap.practice_mode&&snap.classification.count==3);
 assert(!snap.has_personal_best); /* NORMAL and SOLO independent */
 cnk_app_runtime_close(&app);
 cnk_app_runtime second={0};
 assert(!cnk_app_runtime_init(&second,&vfs,3,2));
 assert(!cnk_app_runtime_set_records_path(&second,path));
 assert(cnk_records_find(&second.records,source,2,CNK_RECORDS_PRACTICE,&found)==1);
 assert(found.best_race_ms==fastest);
 cnk_app_runtime_close(&second);
 remove(path);fufs_close(&vfs);
 puts("PASS: solo practice gameplay, zero rivals, Vita triangle mode, checksum+version save, persistent best laps/races, corrupted file rejected, mode isolation, original scene IDs");
 return 0;
}
