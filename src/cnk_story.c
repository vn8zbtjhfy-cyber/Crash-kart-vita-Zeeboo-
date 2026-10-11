#include "cnk_story.h"
#include <string.h>

int cnk_story_init(cnk_story *out,const cnk_track_catalog *catalog,
                   const cnk_story_stage *stages,uint32_t count,
                   uint32_t racers,uint32_t player){
 if(!out||!catalog||!catalog->source||!stages||!count||
    count>CNK_STORY_MAX_STAGES||count>catalog->race_count||
    racers<2||racers>CNK_MAX_RACERS||player>=racers)return -1;
 cnk_story next={0};
 for(uint32_t i=0;i<count;i++){
  const cnk_story_stage *s=&stages[i];
  if(!s->resource_id||s->laps<1||s->laps>999u||
     s->best_required_rank<1||s->best_required_rank>racers)return -2;
  for(uint32_t k=0;k<i;k++)if(stages[k].resource_id==s->resource_id)return -2;
  uint32_t found=0;
  for(uint32_t j=0;j<catalog->race_count;j++){
   const cnk_track_entry *track=cnk_track_race_at(catalog,j);
   if(track && track->resource_id==s->resource_id){
    next.stages[i]=(cnk_story_stage){s->resource_id,j,s->laps,s->best_required_rank};
    found=1;break;
   }
  }
  if(!found)return -3;
 }
 next.active=1;next.stage_count=count;next.racer_count=racers;next.player_slot=player;
 *out=next;return 0;
}
int cnk_story_next(const cnk_story *s,cnk_story_stage *out){
 if(!s||!out||!s->active||!s->stage_count||
    s->stage_count>CNK_STORY_MAX_STAGES||s->completed>s->stage_count)return -1;
 if(s->completed==s->stage_count)return 0;
 *out=s->stages[s->completed];return 1;
}
int cnk_story_complete(cnk_story *s,uint32_t id,const cnk_game_classification *board){
 if(!s||!s->active||!board||s->completed>=s->stage_count||
    !s->racer_count||s->racer_count>CNK_MAX_RACERS||
    s->player_slot>=s->racer_count||board->count!=s->racer_count)return -1;
 const cnk_story_stage *stage=&s->stages[s->completed];
 if(id!=stage->resource_id||!stage->laps||
    stage->best_required_rank>s->racer_count)return -2;
 uint32_t slots=0,ranks=0,player_rank=0;
 for(uint32_t i=0;i<board->count;i++){
  const cnk_game_classification_row *row=&board->rows[i];
  if(!row->finished||row->racer_slot>=s->racer_count||
     row->rank<1||row->rank>s->racer_count||
     !row->total_laps||row->total_laps!=stage->laps||
     row->completed_laps!=row->total_laps||
     (slots&(1u<<row->racer_slot))||
     (ranks&(1u<<(row->rank-1u))))return -3;
  slots|=1u<<row->racer_slot;
  ranks|=1u<<(row->rank-1u);
  if(row->racer_slot==s->player_slot)player_rank=row->rank;
 }
 if(slots!=((1u<<s->racer_count)-1u)||
    ranks!=((1u<<s->racer_count)-1u)||!player_rank)return -4;
 if(player_rank>stage->best_required_rank)return 0;
 cnk_story next=*s;
 next.achieved_ranks[next.completed]=(uint8_t)player_rank;
 next.completed++;
 *s=next;
 return 1;
}
