#define _POSIX_C_SOURCE 200809L
#include "cnk_vita_source_path.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_le32(unsigned char *p,uint32_t x){
 p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);
 p[2]=(unsigned char)(x>>16);p[3]=(unsigned char)(x>>24);
}
/* Make an original-format tiny archive, not a source-asset substitute. */
static void write_archive(const char *path,const char *magic,uint32_t count,
                          uint32_t offset,uint32_t size,size_t length){
 unsigned char b[128]={0};
 assert(length<=sizeof(b));
 memcpy(b,magic,4);write_le32(b+8,count);
 write_le32(b+12,offset);write_le32(b+16,0x12345678u);write_le32(b+20,size);
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(b,1,length,f)==length);
 assert(fclose(f)==0);
}
int main(int argc,char **argv){
 char root[]="/tmp/cnk_source_path_XXXXXX";
 assert(mkdtemp(root));
 char first[256],second[256],third[256];
 assert(snprintf(first,sizeof(first),"%s/first.vfs",root)>0);
 assert(snprintf(second,sizeof(second),"%s/second.vfs",root)>0);
 assert(snprintf(third,sizeof(third),"%s/third.vfs",root)>0);
 const char *paths[]={first,second,third};
 assert(!cnk_vita_source_select(NULL,3));
 assert(!cnk_vita_source_select(paths,0));
 assert(!cnk_vita_source_select(paths,3));
 /* Reject unrelated files, truncated tables and invalid sizes. */
 write_archive(first,"NOT!",1,24,4,28);
 write_archive(second,"FUFS",1,24,4,28);
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",1,24,4,12);
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",50001,24,4,28);
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",1,12,4,28); /* points into table */
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",1,24,999,28); /* past end */
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",0,24,4,28); /* no resource table */
 assert(cnk_vita_source_select(paths,3)==second);
 write_archive(first,"FUFS",1,24,4,28);
 assert(cnk_vita_source_select(paths,3)==first);
 write_archive(third,"FUFS",1,24,4,28);
 assert(cnk_vita_source_select(paths,3)==first);
 const char *with_nulls[]={NULL,"",second};
 assert(cnk_vita_source_select(with_nulls,3)==second);
 /* If the developer provided the source archive locally, verify its real
  * header/table are accepted, without uploading it to the repository. */
 if(argc>1 && access(argv[1],R_OK)==0){
  const char *original[]={argv[1]};
  assert(cnk_vita_source_select(original,1)==original[0]);
 }
 assert(unlink(first)==0);assert(unlink(second)==0);assert(unlink(third)==0);
 assert(rmdir(root)==0);
 puts("VITA SOURCE PATH PASS: archive table bounds, corruption fallbacks, priority");
 return 0;
}
