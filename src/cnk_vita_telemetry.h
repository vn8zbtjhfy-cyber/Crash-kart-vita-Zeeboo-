#ifndef CNK_VITA_TELEMETRY_H
#define CNK_VITA_TELEMETRY_H
#include <stdint.h>
/* Lightweight post-vblank performance measurements for future console QA.
 * Does not read personal identifiers, ROM contents or game save files. */
typedef struct {
 uint64_t presented_frames;
 uint64_t measured_intervals;
 uint64_t sum_interval_us;
 uint64_t last_timestamp_us;
 uint64_t longest_interval_us;
 uint64_t slow_intervals;
 uint64_t clock_rebases;
 uint64_t suspension_gaps;
 unsigned have_timestamp;
} cnk_vita_telemetry;
void cnk_vita_telemetry_present(cnk_vita_telemetry *t, uint64_t now_us);
int cnk_vita_telemetry_write(const char *path,const cnk_vita_telemetry *t,int exit_code);
#endif
