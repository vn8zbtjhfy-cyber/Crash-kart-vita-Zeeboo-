#define _POSIX_C_SOURCE 200809L
#include "cnk_vita_telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void){
 cnk_vita_telemetry t={0};
 cnk_vita_telemetry_present(NULL,100);
 assert(cnk_vita_telemetry_write(NULL,&t,0)<0);
 assert(cnk_vita_telemetry_write("",&t,0)<0);
 assert(cnk_vita_telemetry_write("/tmp/cnk_no_such_dir_telemetry/report",&t,0)<0);
 cnk_vita_telemetry_present(&t,1000000);
 cnk_vita_telemetry_present(&t,1016667);
 cnk_vita_telemetry_present(&t,1056667); /* 40 ms stutter */
 cnk_vita_telemetry_present(&t,1056667); /* duplicate timestamp ignored */
 cnk_vita_telemetry_present(&t,900000); /* clock correction */
 cnk_vita_telemetry_present(&t,6900001); /* suspended 6 seconds */
 cnk_vita_telemetry_present(&t,6916668);
 assert(t.presented_frames==7 && t.measured_intervals==3);
 assert(t.sum_interval_us==73334 && t.longest_interval_us==40000);
 assert(t.slow_intervals==1 && t.clock_rebases==1 && t.suspension_gaps==1);
 char name[]="/tmp/cnk_vita_perf_XXXXXX";
 int fd=mkstemp(name);assert(fd>=0);assert(close(fd)==0);
 assert(cnk_vita_telemetry_write(name,&t,-13)==0);
 FILE *f=fopen(name,"rb");assert(f);
 char buf[640]={0};size_t len=fread(buf,1,sizeof(buf)-1,f);
 assert(len>0 && !ferror(f) && fclose(f)==0);
 assert(strstr(buf,"CNK_VITA_TELEMETRY_V1\n"));
 assert(strstr(buf,"exit_code=-13\n"));
 assert(strstr(buf,"presented_frames=7\n"));
 assert(strstr(buf,"measured_intervals=3\n"));
 assert(strstr(buf,"average_frame_us=24444\n"));
 assert(strstr(buf,"longest_frame_us=40000\n"));
 assert(strstr(buf,"frames_over_33ms=1\n"));
 assert(strstr(buf,"clock_rebases=1\n"));
 assert(strstr(buf,"suspension_gaps=1\n"));
 assert(unlink(name)==0);
 puts("VITA TELEMETRY PASS: intervals, stutters, suspend, clock reset, concise report");
 return 0;
}
