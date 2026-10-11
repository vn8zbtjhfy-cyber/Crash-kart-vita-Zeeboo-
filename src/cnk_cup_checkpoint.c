#include "cnk_cup_checkpoint.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Fixed-size, LE, versioned, pointer-free checkpoint. No host struct dump.
 * Header: magic(8), version(4), rounds(4), racers(4), completed(4), crc(4).
 * Data: scene resource IDs(12*4), score scale(6*4), rank grid(12*6).
 * CRC covers header version/counts and all data, except the CRC field. */
#define HEADER 28u
#define IDS_SIZE (CNK_CUP_MAX_ROUNDS*4u)
#define POINTS_SIZE (CNK_MAX_RACERS*4u)
#define RANKS_SIZE (CNK_CUP_MAX_ROUNDS*CNK_MAX_RACERS)
#define FILE_SIZE (HEADER+IDS_SIZE+POINTS_SIZE+RANKS_SIZE)
#define IDS_OFF HEADER
#define POINTS_OFF (IDS_OFF+IDS_SIZE)
#define RANKS_OFF (POINTS_OFF+POINTS_SIZE)
static void put32(unsigned char *dst,uint32_t n){
 for(unsigned j=0;j<4;j++)dst[j]=(unsigned char)(n>>(8u*j));
}
static uint32_t get32(const unsigned char *src){
 return (uint32_t)src[0]|((uint32_t)src[1]<<8)|
        ((uint32_t)src[2]<<16)|((uint32_t)src[3]<<24);
}
static uint32_t hash(const unsigned char *data,size_t n){
 uint32_t crc=0xffffffffu;
 for(size_t i=0;i<n;i++){
  crc^=data[i];
  for(unsigned k=0;k<8;k++)crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
 }
 return ~crc;
}
static uint32_t checksum(const unsigned char *data){
 unsigned char digest_input[FILE_SIZE-12u];
 memcpy(digest_input,data+8,16u);
 memcpy(digest_input+16,data+HEADER,FILE_SIZE-HEADER);
 return hash(digest_input,sizeof(digest_input));
}
/* Rebuild all totals from unique rank permutations and the manifest.
 * Passing fabricated sums or unrelated Cup state is never sufficient. */
static int replay(cnk_cup *cup,const cnk_cup *source,int check_totals){
 if(!cup||!source||!source->active||
    source->round_count!=cup->round_count||
    source->racer_count!=cup->racer_count||
    source->completed_rounds>cup->round_count)return -1;
 for(uint32_t i=0;i<source->round_count;i++){
  if(source->resource_ids[i]!=cup->resource_ids[i])return -1;
 }
 for(uint32_t i=0;i<source->racer_count;i++){
  if(source->points_by_rank[i]!=cup->points_by_rank[i])return -1;
 }
 for(uint32_t r=0;r<CNK_CUP_MAX_ROUNDS;r++){
  cnk_game_classification board={0};
  board.count=cup->racer_count;
  for(uint32_t j=0;j<CNK_MAX_RACERS;j++){
   uint32_t rank=source->round_ranks[r][j];
   if(r>=source->completed_rounds || j>=cup->racer_count){
    if(rank)return -1;
    continue;
   }
   board.rows[j].racer_slot=j;
   board.rows[j].rank=rank;
   board.rows[j].finished=1;
   board.rows[j].completed_laps=board.rows[j].total_laps=1;
  }
  if(r<source->completed_rounds &&
     cnk_cup_commit(cup,cup->resource_ids[r],&board))return -1;
 }
 if(cup->completed_rounds!=source->completed_rounds)return -1;
 if(check_totals)for(uint32_t j=0;j<CNK_MAX_RACERS;j++){
  if(source->total_points[j]!=cup->total_points[j] ||
     source->rank_sum[j]!=cup->rank_sum[j] ||
     source->last_ranks[j]!=cup->last_ranks[j])return -1;
 }
 return 0;
}
/* Copy only a byte-exact, CRC-valid previous checkpoint into a sidecar.
 * The loader separately verifies scene IDs, rounds, scoring and rank history
 * against the CURRENT developer manifest before accepting this backup. */
static int backup_valid_primary(const char *path){
 FILE *src=fopen(path,"rb");
 if(!src)return 0; /* first save or already damaged file: do not back it up */
 unsigned char old[FILE_SIZE];
 size_t count=fread(old,1,sizeof(old),src);
 int extra=fgetc(src),read_error=ferror(src);
 if(fclose(src))read_error=1;
 if(read_error||count!=sizeof(old)||extra!=EOF||
    memcmp(old,"CNKCUP01",8)||get32(old+8)!=1u||
    get32(old+24)!=checksum(old))return 0;
 char backup[CNK_CUP_CHECKPOINT_PATH_MAX+8u];
 char temp[CNK_CUP_CHECKPOINT_PATH_MAX+12u];
 snprintf(backup,sizeof(backup),"%s.bak",path);
 snprintf(temp,sizeof(temp),"%s.bak.tmp",path);
 FILE *out=fopen(temp,"wb");
 if(!out)return -1;
 int bad=fwrite(old,1,sizeof(old),out)!=sizeof(old);
 if(fflush(out))bad=1;
 if(fclose(out))bad=1;
 if(bad){remove(temp);return -1;}
 if(rename(temp,backup)){remove(temp);return -1;}
 return 0;
}
int cnk_cup_checkpoint_save(const cnk_cup *cup,const char *path){
 if(!cup||!path||!path[0]||strlen(path)>CNK_CUP_CHECKPOINT_PATH_MAX-5u||
    !cup->active||!cup->completed_rounds||
    cup->round_count<2||cup->round_count>CNK_CUP_MAX_ROUNDS||
    cup->racer_count<2||cup->racer_count>CNK_MAX_RACERS)return -1;
 /* Check historical score consistency without a source catalog; the write
  * still uses only explicitly verified IDs already in the Cup. */
 cnk_cup rebuilt=*cup;
 memset(rebuilt.total_points,0,sizeof(rebuilt.total_points));
 memset(rebuilt.rank_sum,0,sizeof(rebuilt.rank_sum));
 memset(rebuilt.last_ranks,0,sizeof(rebuilt.last_ranks));
 memset(rebuilt.round_ranks,0,sizeof(rebuilt.round_ranks));
 rebuilt.completed_rounds=0;
 for(uint32_t i=0;i<cup->round_count;i++){
  if(!cup->resource_ids[i])return -1;
  for(uint32_t j=0;j<i;j++)if(cup->resource_ids[j]==cup->resource_ids[i])return -1;
 }
 for(uint32_t i=0;i<cup->racer_count;i++){
  if(!cup->points_by_rank[i] ||
     (i && cup->points_by_rank[i]>=cup->points_by_rank[i-1]))return -1;
 }
 if(replay(&rebuilt,cup,1))return -1;
 unsigned char data[FILE_SIZE]={0};
 memcpy(data,"CNKCUP01",8);
 put32(data+8,1u);
 put32(data+12,cup->round_count);
 put32(data+16,cup->racer_count);
 put32(data+20,cup->completed_rounds);
 for(uint32_t i=0;i<cup->round_count;i++)put32(data+IDS_OFF+4u*i,cup->resource_ids[i]);
 for(uint32_t i=0;i<cup->racer_count;i++)put32(data+POINTS_OFF+4u*i,cup->points_by_rank[i]);
 for(uint32_t i=0;i<CNK_CUP_MAX_ROUNDS;i++)
  for(uint32_t j=0;j<CNK_MAX_RACERS;j++)
   data[RANKS_OFF+i*CNK_MAX_RACERS+j]=cup->round_ranks[i][j];
 put32(data+24,checksum(data));
 char tmp[CNK_CUP_CHECKPOINT_PATH_MAX+8u];
 snprintf(tmp,sizeof(tmp),"%s.tmp",path);
 FILE *f=fopen(tmp,"wb");if(!f)return -2;
 int bad=fwrite(data,1,sizeof(data),f)!=sizeof(data);
 if(fflush(f))bad=1;
 if(fclose(f))bad=1;
 if(bad){remove(tmp);return -3;}
 if(backup_valid_primary(path)){remove(tmp);return -5;}
 if(rename(tmp,path)){remove(tmp);return -4;}
 return 0;
}
int cnk_cup_checkpoint_load(cnk_cup *out,const char *path,
                     const cnk_cup_manifest *manifest,
                     const cnk_track_catalog *catalog){
 if(!out||!path||!path[0]||!manifest||!catalog||
    strlen(path)>CNK_CUP_CHECKPOINT_PATH_MAX-5u)return -1;
 FILE *f=fopen(path,"rb");if(!f)return errno==ENOENT?-2:-3;
 unsigned char data[FILE_SIZE];
 size_t n=fread(data,1,sizeof(data),f);
 int extra=fgetc(f),bad=ferror(f);
 if(fclose(f))bad=1;
 if(bad||n!=sizeof(data)||extra!=EOF || memcmp(data,"CNKCUP01",8)||
    get32(data+8)!=1u||get32(data+24)!=checksum(data))return -3;
 cnk_cup result={0};
 if(cnk_cup_init(&result,catalog,manifest->resource_ids,manifest->round_count,
                 manifest->points,manifest->racer_count))return -4;
 if(get32(data+12)!=result.round_count ||
    get32(data+16)!=result.racer_count ||
    get32(data+20)==0u ||get32(data+20)>result.round_count)return -4;
 for(uint32_t i=0;i<CNK_CUP_MAX_ROUNDS;i++){
  uint32_t id=get32(data+IDS_OFF+4u*i);
  if(id!=(i<result.round_count?result.resource_ids[i]:0u))return -4;
 }
 for(uint32_t i=0;i<CNK_MAX_RACERS;i++){
  uint32_t points=get32(data+POINTS_OFF+4u*i);
  if(points!=(i<result.racer_count?result.points_by_rank[i]:0u))return -4;
 }
 cnk_cup stored=result;
 stored.completed_rounds=get32(data+20);
 for(uint32_t i=0;i<CNK_CUP_MAX_ROUNDS;i++)
  for(uint32_t j=0;j<CNK_MAX_RACERS;j++)
   stored.round_ranks[i][j]=data[RANKS_OFF+i*CNK_MAX_RACERS+j];
 if(replay(&result,&stored,0))return -5;
 *out=result;
 return 0;
}

int cnk_cup_checkpoint_load_backup(cnk_cup *out,const char *path,
                     const cnk_cup_manifest *manifest,
                     const cnk_track_catalog *catalog){
 if(!out||!path||!path[0]||!manifest||!catalog||
    strlen(path)>CNK_CUP_CHECKPOINT_PATH_MAX-5u)return -1;
 char backup[CNK_CUP_CHECKPOINT_PATH_MAX+8u];
 snprintf(backup,sizeof(backup),"%s.bak",path);
 return cnk_cup_checkpoint_load(out,backup,manifest,catalog);
}
