#include "cnk_vita_telemetry.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

/* Deltas exceeding five seconds are almost certainly app suspension,
 * not an ordinary game frame. Excluding them avoids bogus FPS reports. */
#define CNK_SUSPENSION_THRESHOLD_US UINT64_C(5000000)
#define CNK_SLOW_FRAME_THRESHOLD_US UINT64_C(33333)

void cnk_vita_telemetry_present(cnk_vita_telemetry *t,uint64_t now_us){
 if(!t)return;
 if(t->have_timestamp){
  if(now_us<t->last_timestamp_us){
   t->clock_rebases++;
  }else if(now_us>t->last_timestamp_us){
   uint64_t delta=now_us-t->last_timestamp_us;
   if(delta>CNK_SUSPENSION_THRESHOLD_US){
    t->suspension_gaps++;
   }else{
    t->measured_intervals++;
    if(UINT64_MAX-t->sum_interval_us<delta)t->sum_interval_us=UINT64_MAX;
    else t->sum_interval_us+=delta;
    if(delta>t->longest_interval_us)t->longest_interval_us=delta;
    if(delta>CNK_SLOW_FRAME_THRESHOLD_US)t->slow_intervals++;
   }
  }
 }
 t->have_timestamp=1;
 t->last_timestamp_us=now_us;
 t->presented_frames++;
}

int cnk_vita_telemetry_write(const char *path,const cnk_vita_telemetry *t,int exit_code){
 if(!path||!*path||!t)return -1;
 FILE *fp=fopen(path,"wb");
 if(!fp)return -2;
 uint64_t avg=t->measured_intervals?t->sum_interval_us/t->measured_intervals:0;
 int n=fprintf(fp,
  "CNK_VITA_TELEMETRY_V1\nexit_code=%d\n"
  "presented_frames=%" PRIu64 "\nmeasured_intervals=%" PRIu64 "\n"
  "average_frame_us=%" PRIu64 "\nlongest_frame_us=%" PRIu64 "\n"
  "frames_over_33ms=%" PRIu64 "\nclock_rebases=%" PRIu64 "\n"
  "suspension_gaps=%" PRIu64 "\n",
  exit_code,t->presented_frames,t->measured_intervals,avg,
  t->longest_interval_us,t->slow_intervals,t->clock_rebases,
  t->suspension_gaps);
 int closed=fclose(fp);
 return n<0||closed!=0?-3:0;
}
