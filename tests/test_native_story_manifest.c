#include "cnk_story_manifest.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void write_file(const char *path,const void *data,size_t n){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(data,1,n,f)==n);assert(!fclose(f));
}
static void write_text(const char *path,const char *s){
 write_file(path,s,strlen(s));
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 assert(cat.race_count>=3);
 uint32_t id0=cnk_track_race_at(&cat,0)->resource_id;
 uint32_t id1=cnk_track_race_at(&cat,2)->resource_id;
 char good[256];
 snprintf(good,sizeof(good),"# DEVELOPMENT ONLY - not original chapters\n"
  "CNK_STORY_DEV_V1\nSTAGE %u 2 1\nSTAGE %u 3 2\n",id0,id1);
 write_text(argv[2],good);
 cnk_story_manifest cfg={0};
 assert(!cnk_story_manifest_read(argv[2],&cat,3,&cfg));
 assert(cfg.stage_count==2&&cfg.stages[1].race_ordinal==2);
 cnk_story_manifest saved=cfg;
 const char *invalid[]={
  "CNK_STORY_DEV_V1\n",
  "CNK_STORY_DEV_V1\nSTAGE -1 2 1\n",
  "CNK_STORY_DEV_V1\nSTAGE 4294967296 2 1\n",
  "CNK_STORY_DEV_V1\nSTAGE 4 3 2\nTRAILER GARBAGE\n",
  "CNK_STORY_DEV_V1\nSTAGE 4 2 1 17\n",
  "CNK_STORY_DEV_V1\nSTAGE 4 0 1\n",
  "CNK_STORY_DEV_V1\nSTAGE 4 2 9999\n",
  "NOT_A_STORY\nSTAGE 4 2 1\n",
  "CNK_STORY_DEV_V1\nSTAGE 4 2 1\nSTAGE 4 2 1\n"
 };
 for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){
  write_text(argv[2],invalid[i]);
  assert(cnk_story_manifest_read(argv[2],&cat,3,&cfg)<0);
  assert(!memcmp(&cfg,&saved,sizeof(cfg)));
 }
 const char nul[]="CNK_STORY_DEV_V1\nSTAGE 3 2 1\0STAGE 4 2 1\n";
 write_file(argv[2],nul,sizeof(nul)-1);
 assert(cnk_story_manifest_read(argv[2],&cat,3,&cfg)<0);
 assert(!memcmp(&cfg,&saved,sizeof(cfg)));
 /* No configuration: clean install may still start a regular race. */
 cnk_app_runtime app={0};cnk_app_snapshot shot={0};cnk_driver_input in={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!cnk_app_runtime_snapshot(&app,&shot)&&!shot.story_available);
 cnk_story_manifest broken=saved;broken.stages[0].resource_id=UINT32_MAX;
 assert(cnk_frontend_story_configure(&app.frontend,&broken)<0);
 assert(!app.frontend.story_manifest_ready);
 assert(!cnk_frontend_story_configure(&app.frontend,&saved));
 assert(!cnk_app_runtime_snapshot(&app,&shot)&&shot.story_available);
 assert(!cnk_app_runtime_tick(&app,1000000u,0,in,&shot));
 assert(shot.state==CNK_FRONTEND_SELECT);
 assert(!cnk_app_runtime_tick(&app,1016667u,CNK_UI_PAUSE,in,&shot));
 assert(shot.state==CNK_FRONTEND_RACE && shot.story_active &&
        shot.story_total==2 && shot.configured_laps==2 &&
        shot.source_resource_id==id0 &&
        (shot.app_events&CNK_APP_EVENT_RACE_STARTED));
 assert(!cnk_app_runtime_tick(&app,1033334u,CNK_UI_PAUSE,in,&shot));
 assert(shot.state==CNK_FRONTEND_RACE); /* START held: no duplicate */
 cnk_cup_manifest no_cup={0};
 assert(cnk_frontend_cup_configure(&app.frontend,&no_cup)<0);
 cnk_app_runtime_close(&app);
 write_text(argv[2],good);
 assert(!cnk_story_manifest_read(argv[2],&cat,3,&cfg));
 assert(!remove(argv[2]));
 fufs_close(&vfs);
 puts("STORY MANIFEST PASS: authentic track IDs, validated objectives, binary rejection, START launches source race");
 return 0;
}
