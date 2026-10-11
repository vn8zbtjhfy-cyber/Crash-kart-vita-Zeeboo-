#include "cnk_records.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Versioned 20-byte header + 28-byte LE slots, CRC-32 independent of ABI.
 * V1 from advance 56 uses 32 slots; V2 uses 128 and reads V1 unchanged. */
#define HEADER_SIZE 20u
#define ENTRY_SIZE 28u
#define LEGACY_LIMIT 32u
#define DISK_SIZE (HEADER_SIZE+ENTRY_SIZE*CNK_RECORDS_LIMIT)
static void put32(unsigned char *p,uint32_t v){
 for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(i*8));
}
static void put64(unsigned char *p,uint64_t v){
 for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(v>>(i*8));
}
static uint32_t get32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint64_t get64(const unsigned char *p){
 uint64_t n=0;
 for(unsigned i=0;i<8;i++)n|=((uint64_t)p[i])<<(i*8);
 return n;
}
static uint32_t crc32_bytes(const unsigned char *data,size_t len){
 uint32_t crc=0xffffffffu;
 for(size_t i=0;i<len;i++){
  crc^=data[i];
  for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));
 }
 return ~crc;
}
static int valid_entry(const cnk_record *r){
 return r && r->resource_id && r->laps && r->laps<=999 &&
        r->mode<=CNK_RECORDS_PRACTICE &&
        (r->best_lap_ms || r->best_race_ms) &&
        r->best_lap_ms<=86400000ull && r->best_race_ms<=86400000ull;
}
static int valid_db(const cnk_records *db){
 if(!db||db->count>CNK_RECORDS_LIMIT)return 0;
 for(uint32_t i=0;i<db->count;i++){
  if(!valid_entry(&db->entries[i]))return 0;
  for(uint32_t j=0;j<i;j++)
   if(db->entries[i].resource_id==db->entries[j].resource_id &&
      db->entries[i].laps==db->entries[j].laps &&
      db->entries[i].mode==db->entries[j].mode)return 0;
 }
 return 1;
}
int cnk_records_find(const cnk_records *db,uint32_t id,uint32_t laps,
                      uint32_t mode,cnk_record *out){
 if(!valid_db(db)||!out||!id||!laps||mode>CNK_RECORDS_PRACTICE)return -1;
 for(uint32_t i=0;i<db->count;i++){
  const cnk_record *r=&db->entries[i];
  if(r->resource_id==id&&r->laps==laps&&r->mode==mode){*out=*r;return 1;}
 }
 *out=(cnk_record){0};return 0;
}
int cnk_records_update(cnk_records *db,uint32_t id,uint32_t laps,
                        uint32_t mode,uint64_t lap,uint64_t race){
 if(!valid_db(db)||!id||!laps||laps>999||mode>CNK_RECORDS_PRACTICE ||
    lap>86400000ull||race>86400000ull)return -1;
 if(!lap&&!race)return 0;
 for(uint32_t i=0;i<db->count;i++){
  cnk_record *r=&db->entries[i];
  if(r->resource_id==id && r->laps==laps && r->mode==mode){
   int changed=0;
   if(lap&&(!r->best_lap_ms||lap<r->best_lap_ms)){
    r->best_lap_ms=lap;changed=1;
   }
   if(race&&(!r->best_race_ms||race<r->best_race_ms)){
    r->best_race_ms=race;changed=1;
   }
   return changed;
  }
 }
 if(db->count==CNK_RECORDS_LIMIT)return -2;
 db->entries[db->count++]=(cnk_record){id,laps,mode,lap,race};
 return 1;
}
static int records_write_atomic(const cnk_records *db,const char *path){
 if(!valid_db(db)||!path||!path[0])return -1;
 size_t n=strlen(path);
 if(n>CNK_RECORDS_PATH_LIMIT-5u)return -1;
 unsigned char buffer[DISK_SIZE]={0};
 memcpy(buffer,"CNKREC02",8);
 put32(buffer+8,2u);put32(buffer+12,db->count);
 for(uint32_t i=0;i<db->count;i++){
  const cnk_record *r=&db->entries[i];unsigned char *p=buffer+HEADER_SIZE+i*ENTRY_SIZE;
  put32(p,r->resource_id);put32(p+4,r->laps);put32(p+8,r->mode);
  put64(p+12,r->best_lap_ms);put64(p+20,r->best_race_ms);
 }
 unsigned char protected_bytes[8+ENTRY_SIZE*CNK_RECORDS_LIMIT];
 memcpy(protected_bytes,buffer+8,8);
 memcpy(protected_bytes+8,buffer+HEADER_SIZE,ENTRY_SIZE*CNK_RECORDS_LIMIT);
 put32(buffer+16,crc32_bytes(protected_bytes,sizeof(protected_bytes)));
 char temporary[CNK_RECORDS_PATH_LIMIT+8];
 snprintf(temporary,sizeof(temporary),"%s.tmp",path);
 FILE *f=fopen(temporary,"wb");
 if(!f)return -2;
 int failed=fwrite(buffer,1,sizeof(buffer),f)!=sizeof(buffer);
 if(fflush(f)!=0)failed=1;
 if(fclose(f)!=0)failed=1;
 if(failed){remove(temporary);return -3;}
 if(rename(temporary,path)!=0){remove(temporary);return -4;}
 return 0;
}
/* A successful second save preserves the last checksum-verified primary in
 * a sidecar .bak. A damaged primary must NEVER replace a healthy backup.
 * The primary write remains atomic. No original Zeebo savedata is touched. */
int cnk_records_save(const cnk_records *db,const char *path){
 if(!valid_db(db)||!path||!path[0])return -1;
 size_t n=strlen(path);
 if(n>CNK_RECORDS_PATH_LIMIT-5u)return -1;
 cnk_records previous={0};
 if(cnk_records_load(&previous,path)==0){
  char backup[CNK_RECORDS_PATH_LIMIT+8];
  snprintf(backup,sizeof(backup),"%s.bak",path);
  if(records_write_atomic(&previous,backup))return -5;
 }
 return records_write_atomic(db,path);
}
/* Explicit fallback only: strict cnk_records_load() continues reporting a
 * corrupt primary for diagnosis. This API cannot silently hide corruption. */
int cnk_records_load_backup(cnk_records *db,const char *path){
 if(!db||!path||!path[0]||strlen(path)>CNK_RECORDS_PATH_LIMIT-5u)return -1;
 char backup[CNK_RECORDS_PATH_LIMIT+8];
 snprintf(backup,sizeof(backup),"%s.bak",path);
 return cnk_records_load(db,backup);
}
int cnk_records_load(cnk_records *db,const char *path){
 if(!db||!path||!path[0])return -1;
 FILE *f=fopen(path,"rb");if(!f)return -2;
 unsigned char buffer[DISK_SIZE]={0};
 size_t head=fread(buffer,1,HEADER_SIZE,f);
 uint32_t cap=0;
 if(head==HEADER_SIZE && !memcmp(buffer,"CNKREC02",8) &&
    get32(buffer+8)==2u)cap=CNK_RECORDS_LIMIT;
 else if(head==HEADER_SIZE && !memcmp(buffer,"CNKREC01",8) &&
         get32(buffer+8)==1u)cap=LEGACY_LIMIT;
 if(!cap){fclose(f);return -3;}
 size_t payload=(size_t)ENTRY_SIZE*cap;
 size_t bytes=fread(buffer+HEADER_SIZE,1,payload,f);
 int extra=fgetc(f),bad=ferror(f);
 fclose(f);
 if(bytes!=payload||extra!=EOF||bad||get32(buffer+12)>cap)return -3;
 unsigned char protected_bytes[8+ENTRY_SIZE*CNK_RECORDS_LIMIT];
 memcpy(protected_bytes,buffer+8,8);
 memcpy(protected_bytes+8,buffer+HEADER_SIZE,payload);
 if(get32(buffer+16)!=crc32_bytes(protected_bytes,8+payload))return -3;
 cnk_records tmp={0};tmp.count=get32(buffer+12);
 for(uint32_t i=0;i<tmp.count;i++){
  const unsigned char *p=buffer+HEADER_SIZE+i*ENTRY_SIZE;
  tmp.entries[i]=(cnk_record){get32(p),get32(p+4),get32(p+8),get64(p+12),get64(p+20)};
 }
 if(!valid_db(&tmp))return -4;
 *db=tmp;return 0;
}
