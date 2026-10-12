#include "cnk_vita_source_path.h"
#include <stdio.h>
#include <string.h>

const char *cnk_vita_source_select(const char *const *candidates,size_t count){
 if(!candidates)return NULL;
 for(size_t i=0;i<count;i++){
  const char *path=candidates[i];
  if(!path||!*path)continue;
  FILE *f=fopen(path,"rb");
  if(!f)continue;
  unsigned char header[12];
  size_t n=fread(header,1,sizeof(header),f);
  int close_error=fclose(f);
  if(n==sizeof(header)&&close_error==0&&memcmp(header,"FUFS",4)==0)
   return path;
 }
 return NULL;
}

const char *cnk_vita_source_find(void){
 /* Never package copyrighted data.vfs in the final VPK. Keep the original
  * archive on the owner's ux0 storage instead. The app0 path is legacy only. */
 static const char *const candidates[]={
  "ux0:data/cnk3d/data.vfs",
  "ux0:data/data.vfs",
  "app0:/data.vfs"
 };
 return cnk_vita_source_select(candidates,sizeof(candidates)/sizeof(candidates[0]));
}
