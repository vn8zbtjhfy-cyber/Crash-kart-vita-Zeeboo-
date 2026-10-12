#include "cnk_source_ui.h"
#include "cnk_png.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>

/* Original source file, verified locally (and kept OUTSIDE the repository):
 * 219: 326x158 original CRASH BANDICOOT NITRO KART 3D title PNG.
 * 119: 32x32 original Wumpa fruit icon PNG.
 * The source archive is still a user-owned external runtime dependency.
 * A different Zeebo build MUST pass the byte-exact match before use. */
static const cnk_source_ui_spec known[CNK_SOURCE_UI_COUNT]={
 {219u,0x89b14497u,326u,158u,0x0c1897b1u},
 {119u,0x43bdcefeu,32u,32u,0x5283e7a0u}
};

const cnk_source_ui_spec *cnk_source_ui_original(unsigned kind){
 return kind<CNK_SOURCE_UI_COUNT?&known[kind]:NULL;
}
int cnk_source_ui_verify_png(const unsigned char *data,size_t length,
                             const cnk_source_ui_spec *spec){
 cnk_png_info info={0};
 if(!data||!spec||!length||length>32u*1024u*1024u||
    !spec->width||!spec->height||spec->width>4096u||spec->height>4096u)
  return -1;
 if(cnk_png_inspect(data,length,&info))return -2;
 if(info.width!=spec->width||info.height!=spec->height)return -3;
 if(crc32(crc32(0L,Z_NULL,0),data,(uInt)length)!=spec->raw_png_crc32)
  return -4;
 return 0;
}
int cnk_source_ui_extract(const fufs_file *vfs,unsigned kind,
                          unsigned char **png,size_t *size){
 const cnk_source_ui_spec *spec=cnk_source_ui_original(kind);
 unsigned char *data=NULL;size_t len=0;
 if(!png||!size)return -1;
 *png=NULL;*size=0;
 if(!vfs||!vfs->data||!vfs->entries||!spec||
    spec->vfs_index>=vfs->count||
    vfs->entries[spec->vfs_index].id!=spec->vfs_id)return -2;
 if(fufs_extract(vfs,spec->vfs_index,&data,&len))return -3;
 if(cnk_source_ui_verify_png(data,len,spec)){
  free(data);return -4;
 }
 *png=data;*size=len;return 0;
}
int cnk_source_ui_key_magenta(unsigned char *rgba,size_t pixels,
                              size_t byte_length,size_t *removed){
 if(!rgba||!removed||pixels>SIZE_MAX/4u||byte_length!=pixels*4u)
  return -1;
 size_t n=0;
 for(size_t i=0;i<pixels;i++){
  unsigned char *p=rgba+i*4u;
  if(p[0]==255u&&p[1]==0u&&p[2]==255u){
   /* Color-key transparent RGB are neutralized to avoid pink edges when
    * the GPU linearly filters between opaque artwork and the clear area. */
   p[0]=p[1]=p[2]=p[3]=0;
   n++;
  }
 }
 *removed=n;
 return 0;
}
