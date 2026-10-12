#include "cnk_race_hud_text.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void){
 cnk_game_hud hud={0};
 cnk_race_hud_text result={0};
 assert(cnk_race_hud_text_build(NULL,0,&result)<0);
 assert(cnk_race_hud_text_build(&hud,0,NULL)<0);
 hud.total_laps=3;hud.competitors=3;hud.rank=2;
 assert(!cnk_race_hud_text_build(&hud,0,&result));
 assert(!strcmp(result.position,"2/3"));
 assert(!strcmp(result.lap,"1/3"));
 assert(!strcmp(result.clock,"00:00:00"));
 assert(!strcmp(result.wumpa,"WUMPA x0"));
 hud.elapsed_ms=79327;hud.lap=1;hud.rank=1;
 assert(!cnk_race_hud_text_build(&hud,10,&result));
 assert(!strcmp(result.clock,"01:19:32"));
 assert(!strcmp(result.lap,"2/3"));
 assert(!strcmp(result.position,"1/3"));
 assert(!strcmp(result.wumpa,"WUMPA x10"));
 hud.lap=3;hud.elapsed_ms=3599999;
 assert(!cnk_race_hud_text_build(&hud,4,&result));
 assert(!strcmp(result.clock,"59:59:99"));
 assert(!strcmp(result.lap,"3/3"));
 /* On invalid game data, a previously drawn good frame is never
  * overwritten with nonsense. */
 cnk_race_hud_text good=result;
 hud.rank=0;
 assert(cnk_race_hud_text_build(&hud,4,&result)<0);
 assert(!memcmp(&good,&result,sizeof(result)));
 hud.rank=2;hud.total_laps=0;
 assert(cnk_race_hud_text_build(&hud,4,&result)<0);
 hud.total_laps=3;hud.lap=4;
 assert(cnk_race_hud_text_build(&hud,4,&result)<0);
 hud.lap=1;
 assert(cnk_race_hud_text_build(&hud,11,&result)<0);
 hud.elapsed_ms=UINT64_MAX;
 /* Huge elapsed values must format safely, not wrap or truncate. */
 assert(!cnk_race_hud_text_build(&hud,1,&result));
 assert(strlen(result.clock)<sizeof(result.clock));
 assert(strstr(result.clock,":")!=NULL);
 assert(!strcmp(result.wumpa,"WUMPA x1"));
 puts("PASS: source-referenced race HUD clock, lap, rank, Wumpa and atomic errors");
 return 0;
}
