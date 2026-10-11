#include "cnk_cup_manifest.h"
#include "cnk_frontend_actions.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void write_text(const char *path,const char *text){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(text,1,strlen(text),f)==strlen(text));
 assert(!fclose(f));
}
static cnk_cup_manifest validated_manifest(const char *path,
                                         const cnk_track_catalog *cat){
 uint32_t id0=cnk_track_race_at(cat,0)->resource_id;
 uint32_t id1=cnk_track_race_at(cat,1)->resource_id;
 char buf[500];
 snprintf(buf,sizeof(buf),"# Developer-defined cup; NOT original Zeebo format\n"
          "CNK_CUP_DEV_V1\nTRACK %u\nTRACK %u\nPOINTS 9 5 2\n",id0,id1);
 write_text(path,buf);
 cnk_cup_manifest config={0};
 assert(!cnk_cup_manifest_read(path,cat,3,&config));
 assert(config.round_count==2&&config.racer_count==3);
 assert(config.resource_ids[0]==id0&&config.resource_ids[1]==id1);
 assert(config.points[0]==9&&config.points[1]==5&&config.points[2]==2);
 return config;
}
static void parser_tests(const char *path,const cnk_track_catalog *cat){
 cnk_cup_manifest good=validated_manifest(path,cat), saved=good;
 assert(cnk_cup_manifest_read("file_that_never_exists.cup",cat,3,&good)==-2);
 assert(!memcmp(&good,&saved,sizeof(good)));
 const char *bad[]={
  "NOT_A_CUP\nTRACK 1\nTRACK 2\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK 4294967296\nTRACK 2\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK -1\nTRACK 2\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nTRACK 3\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nTRACK 4\nPOINTS 9 9 2\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nTRACK 4\nPOINTS 9 5 2 1\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nTRACK 4\nPOINTS 9 5 2\nTRACK 5\n",
  "CNK_CUP_DEV_V1\nTRACK 3\nTRACK 4\nPOINTS 9 5 2\nPOINTS 9 5 2\n",
  "CNK_CUP_DEV_V1\nTRACK 429496729600000000000000000000\nTRACK 2\nPOINTS 9 5 2\n"
 };
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
  write_text(path,bad[i]);
  assert(cnk_cup_manifest_read(path,cat,3,&good)<0);
  assert(!memcmp(&good,&saved,sizeof(good)));
 }
 /* A valid prefix followed by hidden binary is not a valid manifest. */
 FILE *binary=fopen(path,"wb");assert(binary);
 const char hidden[]="CNK_CUP_DEV_V1\0TRACK 7\nTRACK 9\nPOINTS 9 5 2\n";
 assert(fwrite(hidden,1,sizeof(hidden)-1,binary)==sizeof(hidden)-1);
 assert(!fclose(binary));
 assert(cnk_cup_manifest_read(path,cat,3,&good)<0);
 assert(!memcmp(&good,&saved,sizeof(good)));
 validated_manifest(path,cat);
}
static void ui_tests(const fufs_file *vfs,const cnk_cup_manifest *config){
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,vfs,3,2));
 assert(!cnk_app_runtime_snapshot(&app,&snap)&&!snap.cup_available);
 cnk_frontend *ui=&app.frontend;
 cnk_frontend_action_state edge={0};
 cnk_driver_input input={0};
 /* A clean install with no verified manifest has no invented Cup. */
 assert(!cnk_frontend_dispatch(ui,&edge,CNK_UI_PAUSE,input,0));
 assert(ui->state==CNK_FRONTEND_SELECT&&!ui->cup.active);
 assert(!cnk_frontend_dispatch(ui,&edge,0,input,0));
 cnk_cup_manifest wrong=*config;wrong.resource_ids[1]=UINT32_MAX;
 assert(cnk_frontend_cup_configure(ui,&wrong)<0&&!ui->cup_manifest_ready);
 assert(!cnk_frontend_cup_configure(ui,config));
 assert(!cnk_app_runtime_snapshot(&app,&snap)&&snap.cup_available);
 /* Practice remains a separate explicit mode: START must not crash it. */
 assert(!cnk_frontend_toggle_practice(ui));
 assert(!cnk_frontend_dispatch(ui,&edge,CNK_UI_PAUSE,input,0));
 assert(ui->state==CNK_FRONTEND_SELECT&&!ui->cup.active);
 assert(!cnk_frontend_toggle_practice(ui));
 assert(!cnk_frontend_dispatch(ui,&edge,0,input,0));
 assert(!cnk_frontend_dispatch(ui,&edge,CNK_UI_PAUSE,input,0));
 assert(ui->state==CNK_FRONTEND_RACE && ui->cup.active &&
        ui->selected_race==ui->cup.race_ordinals[0]);
 assert(cnk_frontend_cup_configure(ui,config)<0);
 /* Held START does not pause the newly begun Cup on the next frame. */
 assert(!cnk_frontend_dispatch(ui,&edge,CNK_UI_PAUSE,input,16667));
 assert(ui->state==CNK_FRONTEND_RACE);
 /* Normal interruption returns to selector without keeping scores. */
 assert(!cnk_frontend_back_to_select(ui));
 assert(!ui->cup.active && ui->cup_manifest_ready &&
        ui->state==CNK_FRONTEND_SELECT);
 assert(!cnk_app_runtime_snapshot(&app,&snap)&&snap.cup_available);
 cnk_app_runtime_close(&app);
 /* End-to-end application tick: START triggers a real source-backed scene,
  * with the normal event and snapshot plumbing, not just a frontend mock. */
 cnk_app_runtime app2={0};
 assert(!cnk_app_runtime_init(&app2,vfs,3,2));
 assert(!cnk_frontend_cup_configure(&app2.frontend,config));
 assert(!cnk_app_runtime_tick(&app2,1000000,0,input,&snap));
 assert(snap.state==CNK_FRONTEND_SELECT && snap.cup_available);
 assert(!cnk_app_runtime_tick(&app2,1016667,CNK_UI_PAUSE,input,&snap));
 assert(snap.state==CNK_FRONTEND_RACE && snap.cup_active &&
        snap.cup_standings.total_rounds==2 &&
        (snap.app_events&CNK_APP_EVENT_RACE_STARTED) &&
        snap.source_resource_id==config->resource_ids[0]);
 assert(!cnk_app_runtime_tick(&app2,1033334,CNK_UI_PAUSE,input,&snap));
 assert(snap.state==CNK_FRONTEND_RACE && snap.cup_active);
 cnk_app_runtime_close(&app2);
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 assert(cat.race_count>=2);
 parser_tests(argv[2],&cat);
 cnk_cup_manifest config=validated_manifest(argv[2],&cat);
 ui_tests(&vfs,&config);
 assert(!remove(argv[2]));
 fufs_close(&vfs);
 puts("CUP MANIFEST PASS: no guessed cup, strict source IDs/scores, atomic rejects, native START control and practice isolation");
 return 0;
}
