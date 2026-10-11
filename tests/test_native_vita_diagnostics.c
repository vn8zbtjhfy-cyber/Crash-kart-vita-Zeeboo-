#define _POSIX_C_SOURCE 200809L
#include "cnk_vita_diagnostics.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void){
 assert(!strcmp(cnk_vita_diagnostic_name(-1),"source_archive_open"));
 assert(!strcmp(cnk_vita_diagnostic_name(-47),"game_frame_render"));
 assert(!strcmp(cnk_vita_diagnostic_name(-48),"gpu_gl_error"));
 assert(!strcmp(cnk_vita_diagnostic_name(0),"normal_exit"));
 assert(!strcmp(cnk_vita_diagnostic_name(-999),"unknown_status"));
 assert(cnk_vita_diagnostic_write(NULL,-47)<0);
 assert(cnk_vita_diagnostic_write("",-47)<0);
 char file[]="/tmp/cnk_diag_XXXXXX";
 int fd=mkstemp(file);assert(fd>=0);assert(!close(fd));
 assert(!cnk_vita_diagnostic_write(file,-48));
 FILE *f=fopen(file,"rb");assert(f);
 char line[256]={0};size_t n=fread(line,1,sizeof(line)-1,f);
 assert(!ferror(f));assert(!fclose(f));
 assert(n==strlen("CNK_NATIVE_STATUS_V1\ncode=-48\nstage=gpu_gl_error\n"));
 assert(!strcmp(line,"CNK_NATIVE_STATUS_V1\ncode=-48\nstage=gpu_gl_error\n"));
 assert(!cnk_vita_diagnostic_write(file,0));
 f=fopen(file,"rb");assert(f);
 memset(line,0,sizeof(line));n=fread(line,1,sizeof(line)-1,f);
 assert(!ferror(f));assert(!fclose(f));
 assert(n==strlen("CNK_NATIVE_STATUS_V1\ncode=0\nstage=normal_exit\n"));
 assert(!strcmp(line,"CNK_NATIVE_STATUS_V1\ncode=0\nstage=normal_exit\n"));
 assert(!unlink(file));
 puts("VITA DIAGNOSTICS PASS: code mapping, error snapshot, normal-exit reset");
 return 0;
}
