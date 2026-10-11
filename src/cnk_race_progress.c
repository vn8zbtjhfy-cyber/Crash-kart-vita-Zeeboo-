#include "cnk_race_progress.h"
#include <math.h>
#include <string.h>

static float wrap_fraction(float f){
 f-=floorf(f);
 return f;
}
int cnk_race_progress_read(const cnk_race *race,float route_fraction,
                           cnk_race_progress *out){
 if(!race||!out||!race->track || !isfinite(route_fraction) ||
    route_fraction<0.0f || route_fraction>1.0f)return -1;
 uint32_t n=race->track->gate_count;
 if(n<2 || n>CNK_MAX_GATES || race->next_gate>=n ||
    race->completed_laps>race->total_laps || !race->total_laps)return -2;
 uint32_t passed=race->next_gate==0?n-1u:race->next_gate-1u;
 float begin=race->track->gates[passed].fraction;
 float end=race->track->gates[(passed+1u)%n].fraction;
 if(!isfinite(begin)||!isfinite(end)||begin<0.0f||begin>=1.0f||
    end<0.0f||end>=1.0f)return -3;
 float span=wrap_fraction(end-begin);
 if(span<=0.000001f)return -4;
 float distance=wrap_fraction(route_fraction-begin);
 cnk_race_progress next={0};
 next.passed_gates=passed;
 /* A projected kart that missed the checkpoint or cut across the track is
  * outside its authorized sector; award zero *interpolated* progress, not a
  * fictional gate. The actual race timer/gates are never modified. */
 if(distance<=span+0.000001f){
  next.sector_fraction=distance/span;
  if(next.sector_fraction>1.0f)next.sector_fraction=1.0f;
 }
 *out=next;
 return 0;
}
