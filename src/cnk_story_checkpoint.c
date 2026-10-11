#include "cnk_story_checkpoint.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Magic(8), version/count/racers/player/completed(5x4), CRC(4),
 * then 12 entries of {original scene ID, laps, required rank},
 * finally 12 awarded ranks. Integer fields are explicitly LE. */
#define HEADER_SIZE 32u
#define STAGES_SIZE (CNK_STORY_MAX_STAGES*12u)
#define TOTAL_SIZE (HEADER_SIZE+STAGES_SIZE+CNK_STORY_MAX_STAGES)
#define STAGES_OFF HEADER_SIZE
#define RANKS_OFF (HEADER_SIZE+STAGES_SIZE)
static void put32(unsigned char *p,uint32_t value){
 for(unsigned j=0;j<4;j++)p[j]=(unsigned char)(value>>(j*8u));
}
static uint32_t get32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t crc32_bytes(const unsigned char *p,size_t n){
 uint32_t crc=~0u;
 for(size_t j=0;j<n;j++){
  crc^=p[j];
  for(unsigned k=0;k<8;k++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
 }
 return ~crc;
}
static uint32_t checksum(const unsigned char *record){
 /* No dependence on host padding/endianness, CRC field excluded. */
 unsigned char content[TOTAL_SIZE-12u];
 memcpy(content,record+8,20u);
 memcpy(content+20,record+HEADER_SIZE,TOTAL_SIZE-HEADER_SIZE);
 return crc32_bytes(content,sizeof(content));
}
static int valid_record(const unsigned char *record){
 return !memcmp(record,"CNKSTR01",8) && get32(record+8)==1u &&
        get32(record+28)==checksum(record);
}
static int read_record(const char *path,unsigned char *record){
 FILE *f=fopen(path,"rb");
 if(!f)return errno==ENOENT?-2:-3;
 size_t n=fread(record,1,TOTAL_SIZE,f);
 int extra=fgetc(f),err=ferror(f);
 if(fclose(f))err=1;
 if(err||n!=TOTAL_SIZE||extra!=EOF||!valid_record(record))return -3;
 return 0;
}
static int safe_path(const char *path){
 return path&&path[0] && strlen(path)<=CNK_STORY_CHECKPOINT_PATH_MAX-5u;
}
/* The previous primary is backed up only when it has a valid checksum.
 * A corrupted primary cannot destroy the last known good backup. */
static int backup_good_primary(const char *path){
 unsigned char data[TOTAL_SIZE];
 if(read_record(path,data))return 0;
 char bak[CNK_STORY_CHECKPOINT_PATH_MAX+8u];
 char tmp[CNK_STORY_CHECKPOINT_PATH_MAX+12u];
 snprintf(bak,sizeof(bak),"%s.bak",path);
 snprintf(tmp,sizeof(tmp),"%s.bak.tmp",path);
 FILE *f=fopen(tmp,"wb");if(!f)return -1;
 int bad=fwrite(data,1,sizeof(data),f)!=sizeof(data);
 if(fflush(f))bad=1;
 if(fclose(f))bad=1;
 if(bad||rename(tmp,bak)){remove(tmp);return -1;}
 return 0;
}
int cnk_story_checkpoint_save(const cnk_story *s,const char *path){
 if(!s||!safe_path(path)||!s->active||!s->completed||
    !s->stage_count||s->stage_count>CNK_STORY_MAX_STAGES||
    s->completed>s->stage_count||s->racer_count<2||
    s->racer_count>CNK_MAX_RACERS||s->player_slot>=s->racer_count)return -1;
 unsigned char record[TOTAL_SIZE]={0};
 memcpy(record,"CNKSTR01",8);
 put32(record+8,1u);put32(record+12,s->stage_count);
 put32(record+16,s->racer_count);put32(record+20,s->player_slot);
 put32(record+24,s->completed);
 for(uint32_t i=0;i<s->stage_count;i++){
  const cnk_story_stage *stage=&s->stages[i];
  if(!stage->resource_id||stage->laps<1||stage->laps>999u||
     stage->best_required_rank<1||stage->best_required_rank>s->racer_count)return -1;
  for(uint32_t k=0;k<i;k++)
   if(stage->resource_id==s->stages[k].resource_id)return -1;
  uint32_t rank=s->achieved_ranks[i];
  if(i<s->completed){if(!rank||rank>stage->best_required_rank)return -1;}
  else if(rank)return -1;
  put32(record+STAGES_OFF+i*12u,stage->resource_id);
  put32(record+STAGES_OFF+i*12u+4,stage->laps);
  put32(record+STAGES_OFF+i*12u+8,stage->best_required_rank);
  record[RANKS_OFF+i]=(unsigned char)rank;
 }
 put32(record+28,checksum(record));
 char tmp[CNK_STORY_CHECKPOINT_PATH_MAX+8u];
 snprintf(tmp,sizeof(tmp),"%s.tmp",path);
 FILE *f=fopen(tmp,"wb");if(!f)return -2;
 int bad=fwrite(record,1,sizeof(record),f)!=sizeof(record);
 if(fflush(f))bad=1;
 if(fclose(f))bad=1;
 if(bad){remove(tmp);return -3;}
 if(backup_good_primary(path)){remove(tmp);return -4;}
 if(rename(tmp,path)){remove(tmp);return -5;}
 return 0;
}
int cnk_story_checkpoint_load(cnk_story *out,const char *path,
                    const cnk_story_manifest *manifest,const cnk_track_catalog *cat,
                    uint32_t racers,uint32_t player){
 if(!out||!safe_path(path)||!manifest||!cat)return -1;
 unsigned char data[TOTAL_SIZE];
 int rc=read_record(path,data);if(rc)return rc;
 cnk_story parsed={0};
 if(cnk_story_init(&parsed,cat,manifest->stages,manifest->stage_count,
                   racers,player))return -4;
 if(get32(data+12)!=parsed.stage_count||get32(data+16)!=racers||
    get32(data+20)!=player||!get32(data+24)||
    get32(data+24)>parsed.stage_count)return -4;
 for(uint32_t i=0;i<CNK_STORY_MAX_STAGES;i++){
  uint32_t id=get32(data+STAGES_OFF+i*12u);
  uint32_t laps=get32(data+STAGES_OFF+i*12u+4);
  uint32_t goal=get32(data+STAGES_OFF+i*12u+8);
  if(id!=(i<parsed.stage_count?parsed.stages[i].resource_id:0u)||
     laps!=(i<parsed.stage_count?parsed.stages[i].laps:0u)||
     goal!=(i<parsed.stage_count?parsed.stages[i].best_required_rank:0u))return -5;
  uint32_t rank=data[RANKS_OFF+i];
  if(i<get32(data+24)){
   if(!rank||rank>parsed.stages[i].best_required_rank)return -5;
  }else if(rank)return -5;
  parsed.achieved_ranks[i]=(uint8_t)rank;
 }
 parsed.completed=get32(data+24);
 *out=parsed;
 return 0;
}
int cnk_story_checkpoint_load_backup(cnk_story *out,const char *path,
                const cnk_story_manifest *manifest,const cnk_track_catalog *cat,
                uint32_t racers,uint32_t player){
 if(!safe_path(path))return -1;
 char bak[CNK_STORY_CHECKPOINT_PATH_MAX+8u];
 snprintf(bak,sizeof(bak),"%s.bak",path);
 return cnk_story_checkpoint_load(out,bak,manifest,cat,racers,player);
}
