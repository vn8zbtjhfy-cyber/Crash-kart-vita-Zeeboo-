#include "cnk_start_sequence.h"
#include <stddef.h>

void cnk_start_sequence_reset(cnk_start_sequence *s){
 if(!s)return;
 s->phase=CNK_START_THREE;
 s->remaining_us=CNK_START_LIGHT_US;
}
int cnk_start_sequence_racing(const cnk_start_sequence *s){
 return s&&(s->phase==CNK_START_GO||s->phase==CNK_START_RUNNING);
}
int cnk_start_sequence_step(cnk_start_sequence *s,uint32_t dt,
                            uint32_t *race_dt,uint32_t *events){
 if(!s||!race_dt||!events)return -1;
 *race_dt=0;*events=0;
 if(s->phase==CNK_START_IDLE)return -2;
 if(s->phase==CNK_START_RUNNING){*race_dt=dt;return 0;}
 while(dt && (s->phase==CNK_START_THREE||s->phase==CNK_START_TWO||
              s->phase==CNK_START_ONE)){
  if(!s->remaining_us)return -3; /* protect inconsistent states */
  uint32_t taken=dt<s->remaining_us?dt:s->remaining_us;
  dt-=taken;
  s->remaining_us-=taken;
  if(s->remaining_us)break;
  if(s->phase==CNK_START_THREE){s->phase=CNK_START_TWO;*events|=CNK_START_EVENT_2;}
  else if(s->phase==CNK_START_TWO){s->phase=CNK_START_ONE;*events|=CNK_START_EVENT_1;}
  else {s->phase=CNK_START_GO;*events|=CNK_START_EVENT_GO;}
  s->remaining_us=s->phase==CNK_START_GO?
                  CNK_START_GO_VISIBLE_US:CNK_START_LIGHT_US;
 }
 /* GO display is cosmetic: the entire post-GO fragment is simulated.
  * Do not subtract its animation time from driving and race clock. */
 if(s->phase==CNK_START_GO){
  *race_dt=dt;
  if(dt>=s->remaining_us){s->phase=CNK_START_RUNNING;s->remaining_us=0;}
  else s->remaining_us-=dt;
 }else if(s->phase==CNK_START_RUNNING)*race_dt=dt;
 return 0;
}
