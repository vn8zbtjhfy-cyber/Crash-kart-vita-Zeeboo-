#include "cnk_start_sequence.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void take(cnk_start_sequence *s,uint32_t delta,
                 uint32_t expected_driving,uint32_t expected_events,
                 cnk_start_phase phase){
 uint32_t drive=UINT32_MAX,events=UINT32_MAX;
 assert(cnk_start_sequence_step(s,delta,&drive,&events)==0);
 assert(drive==expected_driving);
 assert(events==expected_events);
 assert(s->phase==phase);
}
int main(void){
 cnk_start_sequence s={0};
 uint32_t drive=0,events=0;
 assert(cnk_start_sequence_step(&s,1000,&drive,&events)<0);
 cnk_start_sequence_reset(&s);
 assert(s.phase==CNK_START_THREE);
 assert(!cnk_start_sequence_racing(&s));
 take(&s,500000,0,0,CNK_START_THREE);
 take(&s,500000,0,CNK_START_EVENT_2,CNK_START_TWO);
 take(&s,1000000,0,CNK_START_EVENT_1,CNK_START_ONE);
 take(&s,999999,0,0,CNK_START_ONE);
 take(&s,1,0,CNK_START_EVENT_GO,CNK_START_GO);
 assert(cnk_start_sequence_racing(&s));
 take(&s,250000,250000,0,CNK_START_GO);
 take(&s,500000,500000,0,CNK_START_RUNNING);
 take(&s,16667,16667,0,CNK_START_RUNNING);
 /* A transition in the middle of the dt contributes ONLY post-GO time. */
 cnk_start_sequence_reset(&s);
 take(&s,2990000,0,CNK_START_EVENT_2|CNK_START_EVENT_1,CNK_START_ONE);
 take(&s,30000,20000,CNK_START_EVENT_GO,CNK_START_GO);
 /* A single long call crosses all boundaries without duplicate events. */
 cnk_start_sequence_reset(&s);
 take(&s,3600000,600000,CNK_START_EVENT_2|CNK_START_EVENT_1|
      CNK_START_EVENT_GO,CNK_START_GO);
 take(&s,160000,160000,0,CNK_START_RUNNING);
 cnk_start_sequence_reset(&s);
 take(&s,0,0,0,CNK_START_THREE);
 assert(cnk_start_sequence_step(NULL,1,&drive,&events)<0);
 assert(cnk_start_sequence_step(&s,1,NULL,&events)<0);
 assert(cnk_start_sequence_step(&s,1,&drive,NULL)<0);
 puts("PASS: countdown boundaries, partial GO frame, no early motion, one-shot events, restart, invalid states");
 return 0;
}
