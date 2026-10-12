#define _POSIX_C_SOURCE 200809L
#include "cnk_vita_source_path.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_header(const char *path,const char *magic){
 FILE *f=fopen(path,"wb");assert(f);
 unsigned char b[12]={0};
 memcpy(b,magic,4);
 assert(fwrite(b,1,sizeof(b),f)==sizeof(b));
 assert(fclose(f)==0);
}
int main(void){
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
 /* An unrelated archive must not displace an authentic lower-priority VFS. */
 write_header(first,"NOT!");
 write_header(second,"FUFS");
 assert(cnk_vita_source_select(paths,3)==second);
 write_header(third,"FUFS");
 assert(cnk_vita_source_select(paths,3)==second);
 write_header(first,"FUFS");
 assert(cnk_vita_source_select(paths,3)==first);
 const char *with_nulls[]={NULL,"",second};
 assert(cnk_vita_source_select(with_nulls,3)==second);
 assert(unlink(first)==0);
 assert(unlink(second)==0);
 assert(unlink(third)==0);
 assert(rmdir(root)==0);
 puts("VITA SOURCE PATH PASS: external VFS selection, priority, invalid/missing skip");
 return 0;
}
