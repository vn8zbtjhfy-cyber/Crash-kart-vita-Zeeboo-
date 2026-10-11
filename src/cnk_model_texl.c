#include "cnk_model_texl.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static uint32_t le32(const unsigned char *p){
 return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
        ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static int is_printable_filename(const unsigned char *p,size_t len){
 if(len<5 || len>=CNK_TEXL_NAME_CAP)return 0;
 for(size_t i=0;i<len;i++)if(p[i]<32 || p[i]>126 || p[i]=='/' || p[i]=='\\')return 0;
 return 1;
}
/* The ROM has one 3DOB (#70) whose TEXL string list is followed by an
 * 18-byte binary record. Do not treat those bytes as a third filename or
 * claim to interpret their meaning. Recognition is deliberately exact. */
static const unsigned char plant_tail[18]={
 0x00,0x00,0x00,0x28,0x00,0x00,0xff,0xff,0xff,0xff,
 0x00,0x20,0x00,0x00,0xff,0xff,0xff,0xff
};
static int known_plant_tail(const unsigned char *p,size_t remaining,
                            const cnk_model_texl *stage){
 return remaining==sizeof(plant_tail) && stage->count==2 &&
        !strcmp(stage->names[0],"plant3.png") &&
        !strcmp(stage->names[1],"fleshplant.png") &&
        !memcmp(p,plant_tail,sizeof(plant_tail));
}
int cnk_model_texl_parse(const unsigned char *bytes,size_t length,cnk_model_texl *out){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!bytes || length<12 || memcmp(bytes,"PIFF",4) ||
    (uint64_t)le32(bytes+4)+8u!=length || memcmp(bytes+8,"3DOB",4))return -2;
 cnk_model_texl stage={0};
 for(size_t pos=12;pos<length;){
  if(length-pos<8)return -3;
  size_t n=le32(bytes+pos+4);
  if(n>length-pos-8)return -4;
  if(!memcmp(bytes+pos,"TEXL",4)){
   const unsigned char *begin=bytes+pos+8;
   for(size_t i=0;i<n;){
    if(known_plant_tail(begin+i,n-i,&stage)){
     stage.opaque_tail_bytes=(unsigned)sizeof(plant_tail);
     i=n;
     break;
    }
    size_t at=i;
    while(i<n && begin[i]!=0)i++;
    if(i==n)return -5;
    size_t len=i-at;
    if(!is_printable_filename(begin+at,len))return -5;
    if(stage.count>=CNK_TEXL_MAX_NAMES)return -6;
    memcpy(stage.names[stage.count],begin+at,len);
    stage.names[stage.count][len]=0;
    stage.count++;
    i++;
   }
  }
  pos+=8+n;
 }
 *out=stage;return 0;
}
int cnk_model_texl_from_vfs(const fufs_file *vfs,uint32_t index,cnk_model_texl *out){
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!vfs||!vfs->data||!vfs->entries||index>=vfs->count)return -2;
 unsigned char *data=NULL;size_t len=0;
 if(fufs_extract(vfs,index,&data,&len))return -3;
 int result=cnk_model_texl_parse(data,len,out);
 free(data);return result;
}
static int is_png(const char *name){
 size_t len=strlen(name);
 return len>4 && !strcmp(name+len-4,".png");
}
int cnk_model_texl_shared_png(const cnk_model_texl lists[3],
                              char name[CNK_TEXL_NAME_CAP]){
 if(!name)return -1;
 name[0]=0;
 if(!lists)return -2;
 for(unsigned i=0;i<lists[0].count;i++){
  const char *candidate=lists[0].names[i];
  if(!is_png(candidate))continue;
  int found[2]={0,0};
  for(unsigned j=1;j<3;j++)
   for(unsigned k=0;k<lists[j].count;k++)
    if(!strcmp(candidate,lists[j].names[k]))found[j-1]=1;
  if(found[0]&&found[1]){
   memcpy(name,candidate,strlen(candidate)+1);
   return 0;
  }
 }
 return -3;
}
