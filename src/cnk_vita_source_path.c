#include "cnk_vita_source_path.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Match the bounds used by fufs_open(), without loading the entire archive.
 * A corrupt higher-priority file must not hide a valid user archive below it. */
#define CNK_VFS_MAX_BYTES (64L*1024L*1024L)
#define CNK_VFS_MAX_ENTRIES 50000u

static uint32_t read_le32(const unsigned char *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

static int valid_archive_table(FILE *f){
 unsigned char header[12], entry[12];
 long size;
 if(fseek(f,0,SEEK_END)!=0)return 0;
 size=ftell(f);
 if(size<24||size>CNK_VFS_MAX_BYTES||fseek(f,0,SEEK_SET)!=0)return 0;
 if(fread(header,1,sizeof(header),f)!=sizeof(header))return 0;
 if(memcmp(header,"FUFS",4)!=0)return 0;
 uint32_t count=read_le32(header+8);
 if(!count||count>CNK_VFS_MAX_ENTRIES)return 0;
 /* Multiplication is bounded by the count check above. */
 size_t table_end=12u+(size_t)count*12u;
 if(table_end>(size_t)size)return 0;
 for(uint32_t i=0;i<count;i++){
  if(fread(entry,1,sizeof(entry),f)!=sizeof(entry))return 0;
  uint32_t offset=read_le32(entry);
  uint32_t payload_size=read_le32(entry+8);
  if((size_t)offset<table_end||(size_t)offset>(size_t)size||
     (size_t)payload_size>(size_t)size-(size_t)offset)return 0;
 }
 return 1;
}

const char *cnk_vita_source_select(const char *const *candidates,size_t count){
 if(!candidates)return NULL;
 for(size_t i=0;i<count;i++){
  const char *path=candidates[i];
  if(!path||!*path)continue;
  FILE *f=fopen(path,"rb");
  if(!f)continue;
  int valid=valid_archive_table(f);
  if(fclose(f)!=0)valid=0;
  if(valid)return path;
 }
 return NULL;
}

const char *cnk_vita_source_find(void){
 /* Source assets belong to the user and must never be included in the VPK. */
 static const char *const candidates[]={
  "ux0:data/cnk3d/data.vfs",
  "ux0:data/data.vfs",
  "app0:/data.vfs"
 };
 return cnk_vita_source_select(candidates,sizeof(candidates)/sizeof(candidates[0]));
}
