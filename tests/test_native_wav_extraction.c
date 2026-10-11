#define _POSIX_C_SOURCE 200809L
#include "fufs.h"
#include "cnk_audio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static int launch(const char *tool,const char *rom,const char *idx,const char *id,const char *target){
 pid_t pid=fork();assert(pid>=0);
 if(pid==0){execl(tool,tool,rom,idx,id,target,(char*)NULL);_exit(127);}
 int status=0;assert(waitpid(pid,&status,0)==pid);
 return WIFEXITED(status)?WEXITSTATUS(status):128;
}
int main(int argc,char **argv){
 assert(argc==3);
 fufs_file vfs={0};assert(!fufs_open(argv[2],&vfs));
 assert(vfs.count>44&&vfs.entries[44].id==0x19df923d);
 char target[]="/tmp/cnk50_original_wav_XXXXXX";
 int fd=mkstemp(target);assert(fd>=0);assert(!close(fd));
 assert(launch(argv[1],argv[2],"44","19df923d",target)==0);
 uint8_t *original=NULL;size_t bytes=0;
 assert(!fufs_extract(&vfs,44,&original,&bytes));
 FILE *f=fopen(target,"rb");assert(f);
 assert(!fseek(f,0,SEEK_END)&&(size_t)ftell(f)==bytes);
 rewind(f);uint8_t *copy=malloc(bytes);assert(copy);
 assert(fread(copy,1,bytes,f)==bytes&&!fclose(f));
 assert(!memcmp(original,copy,bytes));
 cnk_wav wave={0};assert(!cnk_wav_open(copy,bytes,&wave));
 free(copy);free(original);
 /* Wrong identity, wrong resource type, index overflow: no overwritten file. */
 assert(launch(argv[1],argv[2],"44","00000000",target)!=0);
 assert(launch(argv[1],argv[2],"104","3a77b37b",target)!=0);
 assert(launch(argv[1],argv[2],"4294967295","19df923d",target)!=0);
 f=fopen(target,"rb");assert(f);assert(!fseek(f,0,SEEK_END)&&(size_t)ftell(f)==bytes);assert(!fclose(f));
 assert(!unlink(target));fufs_close(&vfs);
 puts("PASS: original Zeebo WAV extracted byte-identically only with FUFS index+ID; invalid resources do not overwrite output");
 return 0;
}
