#include "cnk_cup.h"
#include <limits.h>
#include <string.h>

int cnk_cup_init(cnk_cup *out,const cnk_track_catalog *cat,
                 const uint32_t *ids,uint32_t n,
                 const uint32_t *points,uint32_t racers){
 if(!out||!cat||!cat->source||!ids||!points||n<2||n>CNK_CUP_MAX_ROUNDS||
    n>cat->race_count||racers<2||racers>CNK_MAX_RACERS)return -1;
 cnk_cup tmp={0};
 for(uint32_t i=0;i<racers;i++){
  if(!points[i] || (i && points[i]>=points[i-1]) ||
     points[i]>UINT32_MAX/n)return -2;
  tmp.points_by_rank[i]=points[i];
 }
 for(uint32_t i=0;i<n;i++){
  uint32_t found=0;
  if(!ids[i])return -3;
  for(uint32_t j=0;j<i;j++)if(ids[j]==ids[i])return -3;
  for(uint32_t j=0;j<cat->race_count;j++){
   const cnk_track_entry *t=cnk_track_race_at(cat,j);
   if(t && t->resource_id==ids[i]){
    tmp.race_ordinals[i]=j;tmp.resource_ids[i]=ids[i];found=1;break;
   }
  }
  if(!found)return -4;
 }
 tmp.active=1;tmp.round_count=n;tmp.racer_count=racers;
 *out=tmp;
 return 0;
}
int cnk_cup_commit(cnk_cup *cup,uint32_t id,
                   const cnk_game_classification *results){
 if(!cup||!cup->active||!results||cup->completed_rounds>=cup->round_count||
    cup->resource_ids[cup->completed_rounds]!=id||
    results->count!=cup->racer_count)return -1;
 uint32_t slots=0, ranks=0;
 cnk_cup next=*cup;
 for(uint32_t i=0;i<results->count;i++){
  const cnk_game_classification_row *r=&results->rows[i];
  if(!r->finished)return -2;
  if(r->racer_slot>=cup->racer_count||r->rank<1||r->rank>cup->racer_count||
     (slots&(1u<<r->racer_slot)) || (ranks&(1u<<(r->rank-1u))) ||
     r->completed_laps!=r->total_laps||!r->total_laps)return -3;
  slots|=1u<<r->racer_slot;
  ranks|=1u<<(r->rank-1u);
  uint32_t p=cup->points_by_rank[r->rank-1u];
  if(next.total_points[r->racer_slot]>UINT32_MAX-p ||
     next.rank_sum[r->racer_slot]>UINT32_MAX-r->rank)return -4;
  next.total_points[r->racer_slot]+=p;
  next.rank_sum[r->racer_slot]+=r->rank;
  next.last_ranks[r->racer_slot]=r->rank;
  next.round_ranks[next.completed_rounds][r->racer_slot]=(uint8_t)r->rank;
 }
 if(slots!=((1u<<cup->racer_count)-1u)||
    ranks!=((1u<<cup->racer_count)-1u))return -5;
 next.completed_rounds++;
 *cup=next;
 return 0;
}
int cnk_cup_next_race(const cnk_cup *cup,uint32_t *ordinal){
 if(!cup||!cup->active||!ordinal||cup->completed_rounds>cup->round_count)return -1;
 if(cup->completed_rounds==cup->round_count)return 0;
 *ordinal=cup->race_ordinals[cup->completed_rounds];
 return 1;
}
int cnk_cup_standings_read(const cnk_cup *cup,cnk_cup_standings *out){
 if(!cup||!out||!cup->active||cup->racer_count<2||
    cup->racer_count>CNK_MAX_RACERS||cup->completed_rounds>cup->round_count)return -1;
 cnk_cup_standings s={0};
 s.count=cup->racer_count;s.completed_rounds=cup->completed_rounds;
 s.total_rounds=cup->round_count;s.finished=cup->completed_rounds==cup->round_count;
 for(uint32_t i=0;i<cup->racer_count;i++){
  s.rows[i]=(cnk_cup_row){i,0,cup->total_points[i],cup->rank_sum[i],cup->last_ranks[i]};
 }
 for(uint32_t i=1;i<s.count;i++){
  cnk_cup_row cur=s.rows[i];uint32_t j=i;
  while(j){
   cnk_cup_row prev=s.rows[j-1];
   if(prev.points>cur.points ||
      (prev.points==cur.points && prev.rank_sum<cur.rank_sum) ||
      (prev.points==cur.points && prev.rank_sum==cur.rank_sum && prev.slot<cur.slot))break;
   s.rows[j]=prev;j--;
  }
  s.rows[j]=cur;
 }
 for(uint32_t i=0;i<s.count;i++)s.rows[i].rank=i+1;
 *out=s;return 0;
}
