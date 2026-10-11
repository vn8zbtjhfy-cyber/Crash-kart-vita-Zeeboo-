#include "cnk_png.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static uint32_t be32(const unsigned char *p){
 return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
}
int cnk_png_inspect(const unsigned char *data,size_t size,cnk_png_info *out){
 static const unsigned char sig[8]={137,80,78,71,13,10,26,10};
 cnk_png_info info={0};size_t at=8;int saw_ihdr=0,saw_idat=0,saw_iend=0;
 if(!data||!out)return -1;
 memset(out,0,sizeof(*out));
 if(size<8||memcmp(data,sig,8))return -2;
 while(at<size){
  if(size-at<12)return -3;
  uint32_t len=be32(data+at);
  if((size_t)len>size-at-12)return -3;
  const unsigned char *kind=data+at+4,*blob=data+at+8;
  uLong crc=crc32(0L,Z_NULL,0);
  crc=crc32(crc,(const Bytef*)kind,4);
  crc=crc32(crc,(const Bytef*)blob,len);
  if((uint32_t)crc!=be32(blob+len))return -4;
  if(!saw_ihdr){
   if(memcmp(kind,"IHDR",4)||len!=13)return -5;
   info.width=be32(blob);info.height=be32(blob+4);
   info.bit_depth=blob[8];info.color_type=blob[9];
   info.compression=blob[10];info.filter=blob[11];info.interlace=blob[12];
   if(!info.width||!info.height||info.width>8192||info.height>8192||
      info.compression||info.filter||info.interlace>1)return -6;
   if(info.color_type!=0&&info.color_type!=2&&info.color_type!=3&&
      info.color_type!=4&&info.color_type!=6)return -6;
   saw_ihdr=1;
  }else if(!memcmp(kind,"IHDR",4))return -5;
  if(!memcmp(kind,"IDAT",4)){if(!saw_ihdr||saw_iend)return -5;saw_idat=1;}
  if(!memcmp(kind,"IEND",4)){
   if(len||!saw_idat||at+12!=size)return -5;
   saw_iend=1;break;
  }
  at+=(size_t)len+12;
 }
 if(!saw_ihdr||!saw_idat||!saw_iend)return -5;
 *out=info;return 0;
}
void cnk_png_catalog_free(cnk_png_catalog *cat){if(cat){free(cat->items);memset(cat,0,sizeof(*cat));}}
int cnk_png_catalog_build(const fufs_file *f,cnk_png_catalog *out){
 cnk_png_catalog result={0};size_t n=0;
 if(!f||!out||!f->data)return -1;
 memset(out,0,sizeof(*out));
 if(f->count>(SIZE_MAX/sizeof(cnk_png_resource)))return -2;
 result.items=(cnk_png_resource*)calloc(f->count?f->count:1,sizeof(*result.items));
 if(!result.items)return -3;
 for(uint32_t i=0;i<f->count;i++){
  unsigned char *data=NULL;size_t size=0;cnk_png_info info;
  if(fufs_extract(f,i,&data,&size)){cnk_png_catalog_free(&result);return -4;}
  if(size>=8&&memcmp(data,"\x89PNG\r\n\x1a\n",8)==0){
   if(cnk_png_inspect(data,size,&info)){free(data);cnk_png_catalog_free(&result);return -5;}
   result.items[n++]=(cnk_png_resource){i,f->entries[i].id,info};
  }
  free(data);
 }
 result.count=n;*out=result;return 0;
}
const cnk_png_resource *cnk_png_find_index(const cnk_png_catalog *cat,uint32_t idx){
 if(!cat)return NULL;
 size_t lo=0,hi=cat->count;
 while(lo<hi){size_t mid=lo+(hi-lo)/2;
  if(cat->items[mid].vfs_index<idx)lo=mid+1;else hi=mid;}
 if(lo<cat->count&&cat->items[lo].vfs_index==idx)return &cat->items[lo];
 return NULL;
}
int cnk_png_binding_check(const cnk_png_catalog *cat,uint32_t count,
                          const cnk_texture_binding *entries,size_t n){
 if(!cat||(!entries&&n)||count>1024||n>count)return -1;
 for(size_t i=0;i<n;i++){
  if(entries[i].texture_index>=count ||
     !cnk_png_find_index(cat,entries[i].vfs_index))return -2;
  for(size_t j=0;j<i;j++)if(entries[i].texture_index==entries[j].texture_index)return -3;
 }
 return 0;
}
