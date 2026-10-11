#include "cnk_results_text.h"
#include <stdio.h>
#include <string.h>

int cnk_results_text_build(const cnk_game_classification *classification,
                           cnk_results_line output[CNK_MAX_RACERS]){
 if(!classification||!output||classification->count<1||
    classification->count>CNK_MAX_RACERS)return -1;
 cnk_results_line next[CNK_MAX_RACERS]={0};
 unsigned seen=0;
 for(uint32_t i=0;i<classification->count;i++){
  const cnk_game_classification_row *r=&classification->rows[i];
  if(r->rank!=i+1u||r->racer_slot>=classification->count||
     (seen&(1u<<r->racer_slot))||!r->total_laps||
     r->completed_laps>r->total_laps||
     (r->finished&&r->completed_laps!=r->total_laps))return -2;
  seen|=1u<<r->racer_slot;
  next[i].local_player=r->racer_slot==0;
  int n=snprintf(next[i].primary,sizeof(next[i].primary),
   "P%u  KART %u  %s  LAPS %u/%u",r->rank,r->racer_slot+1u,
   r->finished?"FINISHED":"RACING",r->completed_laps,r->total_laps);
  if(n<0||(size_t)n>=sizeof(next[i].primary))return -3;
  if(r->finished){
   n=snprintf(next[i].secondary,sizeof(next[i].secondary),
       "FINAL %llu.%03llu  LAST %llu.%03llu  BEST %llu.%03llu",
       (unsigned long long)(r->elapsed_ms/1000u),
       (unsigned long long)(r->elapsed_ms%1000u),
       (unsigned long long)(r->last_lap_ms/1000u),
       (unsigned long long)(r->last_lap_ms%1000u),
       (unsigned long long)(r->best_lap_ms/1000u),
       (unsigned long long)(r->best_lap_ms%1000u));
  }else{
   n=snprintf(next[i].secondary,sizeof(next[i].secondary),
       "STILL RACING  ELAPSED %llu.%03llu  BEST %llu.%03llu",
       (unsigned long long)(r->elapsed_ms/1000u),
       (unsigned long long)(r->elapsed_ms%1000u),
       (unsigned long long)(r->best_lap_ms/1000u),
       (unsigned long long)(r->best_lap_ms%1000u));
  }
  if(n<0||(size_t)n>=sizeof(next[i].secondary))return -3;
 }
 memcpy(output,next,sizeof(next));
 return (int)classification->count;
}
