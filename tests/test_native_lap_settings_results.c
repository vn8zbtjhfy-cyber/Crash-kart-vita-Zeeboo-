#include "cnk_app_runtime.h"
#include "cnk_results_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static void step(cnk_app_runtime *app,uint64_t us,uint32_t keys,cnk_app_snapshot *snap){
 assert(!cnk_app_runtime_tick(app,us,keys,(cnk_driver_input){0},snap));
}
static void put32(unsigned char *p,uint32_t n){
 for(int i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));
}
static void put64(unsigned char *p,uint64_t n){
 for(int i=0;i<8;i++)p[i]=(unsigned char)(n>>(i*8));
}
static uint32_t crc(const unsigned char *p,size_t len){
 uint32_t c=0xffffffffu;
 for(size_t i=0;i<len;i++){
  c^=p[i];
  for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&-(c&1u));
 }
 return ~c;
}
/* Generate an actual old V1-format file, not an archive reformatted by V2. */
static void save_v1(const char *path){
 unsigned char v1[20+28*32]={0};
 memcpy(v1,"CNKREC01",8);
 put32(v1+8,1);put32(v1+12,1);
 put32(v1+20,88001);put32(v1+24,2);put32(v1+28,CNK_RECORDS_NORMAL);
 put64(v1+32,43000);put64(v1+40,90000);
 unsigned char checksum[8+28*32];
 memcpy(checksum,v1+8,8);memcpy(checksum+8,v1+20,28*32);
 put32(v1+16,crc(checksum,sizeof(checksum)));
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(v1,1,sizeof(v1),f)==sizeof(v1));assert(!fclose(f));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot s={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 step(&app,1000000,0,&s);
 assert(s.configured_laps==2 && s.state==CNK_FRONTEND_SELECT);
 step(&app,1010000,CNK_UI_LAPS,&s);
 assert(s.configured_laps==3 && (s.app_events&CNK_APP_EVENT_LAPS_CHANGED));
 step(&app,1020000,CNK_UI_LAPS,&s);
 assert(s.configured_laps==3 && !(s.app_events&CNK_APP_EVENT_LAPS_CHANGED));
 step(&app,1030000,0,&s);
 step(&app,1040000,CNK_UI_LAPS,&s);assert(s.configured_laps==5);
 step(&app,1050000,0,&s);
 step(&app,1060000,CNK_UI_LAPS,&s);assert(s.configured_laps==1);
 step(&app,1070000,0,&s);
 step(&app,1080000,CNK_UI_LAPS,&s);assert(s.configured_laps==2);
 step(&app,1090000,0,&s);
 step(&app,1100000,CNK_UI_LAPS,&s);assert(s.configured_laps==3);
 step(&app,1110000,0,&s);
 step(&app,1120000,CNK_UI_CONFIRM,&s);
 assert(s.state==CNK_FRONTEND_RACE&&s.hud.total_laps==3);
 assert(s.classification.count==3&&s.configured_laps==3);
 /* Laps can't change in-race, in countdown or while paused. */
 step(&app,1130000,0,&s);
 step(&app,1140000,CNK_UI_LAPS,&s);assert(s.configured_laps==3);
 step(&app,1150000,CNK_UI_PAUSE,&s);assert(s.state==CNK_FRONTEND_PAUSE);
 step(&app,1160000,0,&s);
 step(&app,1170000,CNK_UI_LAPS,&s);assert(s.configured_laps==3);
 step(&app,1180000,CNK_UI_CANCEL,&s);assert(s.state==CNK_FRONTEND_SELECT);
 step(&app,1190000,0,&s);
 step(&app,1200000,CNK_UI_LAPS,&s);assert(s.configured_laps==5);
 step(&app,1210000,0,&s);
 step(&app,1220000,CNK_UI_RESTART,&s);assert(s.practice_mode);
 step(&app,1230000,0,&s);
 step(&app,1240000,CNK_UI_CONFIRM,&s);
 assert(s.classification.count==1&&s.hud.total_laps==5);
 cnk_app_runtime_close(&app);

 cnk_game_classification board={0};board.count=2;
 board.rows[0]=(cnk_game_classification_row){.racer_slot=0,.rank=1,
  .completed_laps=3,.total_laps=3,.elapsed_ms=123456,
  .last_lap_ms=40000,.best_lap_ms=39000,.finished=1};
 board.rows[1]=(cnk_game_classification_row){.racer_slot=1,.rank=2,
  .completed_laps=2,.total_laps=3,.elapsed_ms=122000,
  .best_lap_ms=42000,.finished=0};
 cnk_results_line lines[CNK_MAX_RACERS]={0};
 assert(cnk_results_text_build(&board,lines)==2);
 assert(lines[0].local_player&&!lines[1].local_player);
 assert(strstr(lines[0].primary,"FINISHED")&&strstr(lines[0].secondary,"FINAL 123.456"));
 assert(strstr(lines[0].secondary,"BEST 39.000"));
 assert(strstr(lines[1].primary,"RACING")&&strstr(lines[1].secondary,"STILL RACING"));
 assert(!strstr(lines[1].secondary,"FINAL"));
 cnk_results_line backup[CNK_MAX_RACERS];memcpy(backup,lines,sizeof(backup));
 board.rows[1].racer_slot=0;assert(cnk_results_text_build(&board,lines)<0);
 assert(!memcmp(backup,lines,sizeof(backup))); /* atomic invalid input */

 char path[200];snprintf(path,sizeof(path),"/tmp/cnk57_records_%lu.dat",(unsigned long)getpid());
 save_v1(path);
 cnk_records rec={0},loaded={0};cnk_record record={0};
 assert(!cnk_records_load(&loaded,path)&&loaded.count==1);
 assert(cnk_records_find(&loaded,88001,2,CNK_RECORDS_NORMAL,&record)==1);
 assert(record.best_lap_ms==43000&&record.best_race_ms==90000);
 rec=loaded;
 /* All 12 race scenes x 4 native lap settings x 2 game modes require 96. */
 for(uint32_t scene=0;scene<12;scene++)for(uint32_t j=0;j<4;j++)
  for(uint32_t mode=0;mode<2;mode++){
   uint32_t laps=(uint32_t[]){1,2,3,5}[j];
   assert(cnk_records_update(&rec,100000u+scene,laps,mode,35000+scene,99999)==1);
  }
 assert(rec.count==97);
 assert(!cnk_records_save(&rec,path));
 assert(!cnk_records_load(&loaded,path));
 assert(loaded.count==97);
 assert(cnk_records_find(&loaded,88001,2,CNK_RECORDS_NORMAL,&record)==1);
 assert(record.best_race_ms==90000); /* migration never loses old record */
 assert(cnk_records_find(&loaded,100011,5,CNK_RECORDS_PRACTICE,&record)==1);
 assert(record.best_lap_ms==35011);
 /* Refuse trailing bytes and preserve valid in-memory data. */
 FILE *f=fopen(path,"ab");assert(f);assert(fputc('X',f)!=EOF);assert(!fclose(f));
 assert(cnk_records_load(&loaded,path)<0&&loaded.count==97);
 assert(!cnk_records_save(&rec,path));
 assert(!cnk_records_load(&loaded,path));
 for(uint32_t id=1;rec.count<CNK_RECORDS_LIMIT;id++)
  assert(cnk_records_update(&rec,200000+id,1,CNK_RECORDS_NORMAL,10000+id,0)==1);
 assert(rec.count==CNK_RECORDS_LIMIT);
 assert(cnk_records_update(&rec,999999,1,CNK_RECORDS_NORMAL,10000,0)==-2);
 remove(path);
 fufs_close(&vfs);
 puts("PASS: menu 1/2/3/5 laps, independent SOLO/normal settings, authentic race gate integration, richer result lines, 32-slot V1 -> 128-slot V2 save migration, 96 combinations, corruption and capacity bounds");
 return 0;
}
