#include "cnk_race_hud_text.h"
#include <stdio.h>
#include <string.h>

int cnk_race_hud_text_build(const cnk_game_hud *hud,unsigned wumpa,
                            cnk_race_hud_text *out){
 if(!hud||!out||!hud->total_laps||hud->total_laps>999u||
    hud->lap>hud->total_laps||!hud->competitors||
    hud->competitors>CNK_MAX_RACERS||
    !hud->rank||hud->rank>hud->competitors||wumpa>10u)return -1;
 cnk_race_hud_text next={0};
 unsigned shown_lap=hud->lap==hud->total_laps?
                       hud->total_laps:hud->lap+1u;
 /* Zeebo shows MM:SS:centiseconds in its race corner. We use the
  * already-simulated race clock, never the wall-clock of the Vita. */
 unsigned long long minutes=(unsigned long long)(hud->elapsed_ms/60000u);
 unsigned seconds=(unsigned)((hud->elapsed_ms/1000u)%60u);
 unsigned centis=(unsigned)((hud->elapsed_ms%1000u)/10u);
 int a=snprintf(next.position,sizeof(next.position),
                "%u/%u",hud->rank,hud->competitors);
 int b=snprintf(next.lap,sizeof(next.lap),
                "%u/%u",shown_lap,hud->total_laps);
 int c=snprintf(next.clock,sizeof(next.clock),
                "%02llu:%02u:%02u",minutes,seconds,centis);
 int d=snprintf(next.wumpa,sizeof(next.wumpa),
                "WUMPA x%u",wumpa);
 if(a<0||b<0||c<0||d<0||
    (size_t)a>=sizeof(next.position)||
    (size_t)b>=sizeof(next.lap)||
    (size_t)c>=sizeof(next.clock)||
    (size_t)d>=sizeof(next.wumpa))return -2;
 memcpy(out,&next,sizeof(next));
 return 0;
}
