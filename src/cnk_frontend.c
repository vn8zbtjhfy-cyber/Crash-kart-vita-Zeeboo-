#include "cnk_frontend.h"
#include <string.h>
#include <limits.h>

int cnk_frontend_init(cnk_frontend *ui,const fufs_file *vfs,
                      uint32_t racer_count,uint32_t total_laps){
 if(!ui||!vfs||!vfs->data||!racer_count||racer_count>CNK_MAX_RACERS||
    !total_laps||total_laps>999u)return -1;
 if(ui->state!=CNK_FRONTEND_EMPTY)return -2;
 cnk_frontend value={0};
 if(cnk_track_catalog_build(vfs,&value.catalog))return -3;
 if(!value.catalog.race_count)return -4;
 value.racer_count=racer_count;
 value.total_laps=total_laps;
 value.state=CNK_FRONTEND_SELECT;
 *ui=value;
 return 0;
}
void cnk_frontend_close(cnk_frontend *ui){
 if(!ui)return;
 cnk_game_unload(&ui->game);
 memset(ui,0,sizeof(*ui));
}
const cnk_track_entry *cnk_frontend_selected(const cnk_frontend *ui){
 if(!ui||!ui->catalog.source||ui->state==CNK_FRONTEND_EMPTY)return NULL;
 return cnk_track_race_at(&ui->catalog,ui->selected_race);
}
int cnk_frontend_select(cnk_frontend *ui,int direction){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||!ui->catalog.race_count||(ui->cup.active||ui->story.active))return -1;
 if(direction!=1&&direction!=-1)return -2;
 uint32_t n=ui->catalog.race_count;
 if(direction>0)ui->selected_race=(ui->selected_race+1u)%n;
 else ui->selected_race=(ui->selected_race+n-1u)%n;
 return 0;
}
int cnk_frontend_toggle_practice(cnk_frontend *ui){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||(ui->cup.active||ui->story.active))return -1;
 ui->practice_mode=!ui->practice_mode;
 return 0;
}
int cnk_frontend_cycle_laps(cnk_frontend *ui){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||(ui->cup.active||ui->story.active))return -1;
 static const uint32_t options[]={1,2,3,5};
 for(unsigned i=0;i<4;i++)if(ui->total_laps==options[i]){
  ui->total_laps=options[(i+1u)%4u];return 0;
 }
 ui->total_laps=1;return 0;
}
int cnk_frontend_start(cnk_frontend *ui){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||!cnk_frontend_selected(ui))return -1;
 if(ui->story.active){
  cnk_story_stage stage={0};
  if(cnk_story_next(&ui->story,&stage)!=1 ||
     ui->selected_race!=stage.race_ordinal ||
     ui->total_laps!=stage.laps || ui->practice_mode ||
     ui->racer_count!=ui->story.racer_count)return -5;
 }
 if(ui->cup.active){
  uint32_t next=0;
  if(cnk_cup_next_race(&ui->cup,&next)!=1 || next!=ui->selected_race ||
     ui->practice_mode || ui->racer_count!=ui->cup.racer_count)return -3;
 }
 if(cnk_game_load_catalog_race(&ui->game,&ui->catalog,ui->selected_race,
                              (ui->practice_mode?1u:ui->racer_count),ui->total_laps))return -2;
 cnk_start_sequence_reset(&ui->start_sequence);
 cnk_launch_reset(&ui->launch);
 ui->start_events=CNK_START_EVENT_3;
 ui->state=CNK_FRONTEND_RACE;
 ui->starts++;
 return 0;
}
int cnk_frontend_cup_configure(cnk_frontend *ui,const cnk_cup_manifest *config){
 if(!ui || !config || ui->state!=CNK_FRONTEND_SELECT ||
    (ui->cup.active||ui->story.active)||ui->story_manifest_ready ||
    ui->practice_mode || ui->game.loaded ||
    config->racer_count!=ui->racer_count)return -1;
 cnk_cup check={0};
 if(cnk_cup_init(&check,&ui->catalog,config->resource_ids,
                  config->round_count,config->points,config->racer_count))return -2;
 ui->cup_manifest=*config;
 ui->cup_manifest_ready=1;
 return 0;
}
int cnk_frontend_cup_begin(cnk_frontend *ui,const uint32_t *ids,
                           uint32_t count,const uint32_t *points){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||(ui->cup.active||ui->story.active)||ui->practice_mode)return -1;
 cnk_cup prepared={0};
 if(cnk_cup_init(&prepared,&ui->catalog,ids,count,points,ui->racer_count))return -2;
 const uint32_t saved=ui->selected_race;
 ui->selected_race=prepared.race_ordinals[0];
 ui->cup=prepared;
 if(cnk_frontend_start(ui)){
  ui->cup=(cnk_cup){0};ui->selected_race=saved;return -3;
 }
 return 0;
}
int cnk_frontend_story_configure(cnk_frontend *ui,const cnk_story_manifest *config){
 if(!ui||!config||ui->state!=CNK_FRONTEND_SELECT||ui->cup.active||
    ui->story.active||ui->cup_manifest_ready||ui->practice_mode||ui->game.loaded)return -1;
 cnk_story checked={0};
 if(cnk_story_init(&checked,&ui->catalog,config->stages,config->stage_count,
                   ui->racer_count,0))return -2;
 ui->story_manifest=*config;
 for(uint32_t i=0;i<checked.stage_count;i++)ui->story_manifest.stages[i]=checked.stages[i];
 ui->story_manifest_ready=1;
 return 0;
}
int cnk_frontend_story_begin(cnk_frontend *ui,const cnk_story_stage *stages,uint32_t count){
 if(!ui||ui->state!=CNK_FRONTEND_SELECT||ui->cup.active||
    ui->story.active||ui->practice_mode||ui->game.loaded)return -1;
 cnk_story story={0};
 if(cnk_story_init(&story,&ui->catalog,stages,count,ui->racer_count,0))return -2;
 uint32_t old_race=ui->selected_race,old_laps=ui->total_laps;
 ui->story=story;
 ui->selected_race=story.stages[0].race_ordinal;
 ui->total_laps=story.stages[0].laps;
 if(cnk_frontend_start(ui)){
  ui->story=(cnk_story){0};ui->selected_race=old_race;
  ui->total_laps=old_laps;return -3;
 }
 return 0;
}
int cnk_frontend_story_next(cnk_frontend *ui){
 if(!ui||!ui->story.active||ui->state!=CNK_FRONTEND_RESULTS||
    !ui->story_last_attempt_success)return -1;
 cnk_story_stage stage={0};
 int found=cnk_story_next(&ui->story,&stage);
 if(found<=0)return found==0?1:-2;
 cnk_game_unload(&ui->game);
 ui->state=CNK_FRONTEND_SELECT;
 ui->selected_race=stage.race_ordinal;
 ui->total_laps=stage.laps;
 ui->story_last_attempt_success=0;
 return cnk_frontend_start(ui)?-3:0;
}
int cnk_frontend_story_resume(cnk_frontend *ui,const cnk_story *verified){
 if(!ui||!verified||ui->state!=CNK_FRONTEND_SELECT||
    ui->cup.active||ui->story.active||ui->game.loaded||
    ui->practice_mode||!ui->story_manifest_ready||
    !verified->active||!verified->completed||
    verified->completed>=verified->stage_count)return -1;
 cnk_story expected={0};
 if(cnk_story_init(&expected,&ui->catalog,ui->story_manifest.stages,
       ui->story_manifest.stage_count,ui->racer_count,0))return -2;
 if(expected.stage_count!=verified->stage_count||
    expected.racer_count!=verified->racer_count||
    expected.player_slot!=verified->player_slot)return -2;
 for(uint32_t i=0;i<expected.stage_count;i++){
  const cnk_story_stage *a=&expected.stages[i],*b=&verified->stages[i];
  if(a->resource_id!=b->resource_id||a->race_ordinal!=b->race_ordinal||
     a->laps!=b->laps||a->best_required_rank!=b->best_required_rank||
     (i<verified->completed?(!verified->achieved_ranks[i]||
       verified->achieved_ranks[i]>a->best_required_rank):
       verified->achieved_ranks[i]!=0))return -2;
 }
 uint32_t old_race=ui->selected_race,old_laps=ui->total_laps;
 cnk_story_stage next=expected.stages[verified->completed];
 ui->story=*verified;
 ui->selected_race=next.race_ordinal;
 ui->total_laps=next.laps;
 if(cnk_frontend_start(ui)){
  ui->story=(cnk_story){0};ui->selected_race=old_race;
  ui->total_laps=old_laps;return -3;
 }
 return 0;
}
int cnk_frontend_cup_resume(cnk_frontend *ui,const cnk_cup *verified){
 if(!ui||!verified||ui->state!=CNK_FRONTEND_SELECT||ui->cup.active||
    ui->game.loaded||!ui->cup_manifest_ready||ui->practice_mode||
    !verified->active||!verified->completed_rounds||
    verified->completed_rounds>=verified->round_count||
    verified->round_count!=ui->cup_manifest.round_count||
    verified->racer_count!=ui->cup_manifest.racer_count)return -1;
 for(uint32_t i=0;i<verified->round_count;i++)
  if(verified->resource_ids[i]!=ui->cup_manifest.resource_ids[i])return -2;
 for(uint32_t i=0;i<verified->racer_count;i++)
  if(verified->points_by_rank[i]!=ui->cup_manifest.points[i])return -2;
 uint32_t next=0;
 if(cnk_cup_next_race(verified,&next)!=1 || next>=ui->catalog.race_count)return -3;
 const cnk_track_entry *entry=cnk_track_race_at(&ui->catalog,next);
 if(!entry||entry->resource_id!=verified->resource_ids[verified->completed_rounds])return -3;
 uint32_t old_selected=ui->selected_race;
 ui->selected_race=next;
 ui->cup=*verified;
 if(cnk_frontend_start(ui)){
  ui->cup=(cnk_cup){0};ui->selected_race=old_selected;return -4;
 }
 return 0;
}
int cnk_frontend_cup_next(cnk_frontend *ui){
 if(!ui||!ui->cup.active||ui->state!=CNK_FRONTEND_RESULTS)return -1;
 uint32_t ordinal=0;
 int next=cnk_cup_next_race(&ui->cup,&ordinal);
 if(next<=0)return next==0?1:-2;
 cnk_game_unload(&ui->game);
 ui->state=CNK_FRONTEND_SELECT;
 ui->selected_race=ordinal;
 /* A failed source load is retryable from SELECT without consuming a round. */
 if(cnk_frontend_start(ui))return -3;
 return 0;
}
int cnk_frontend_toggle_pause(cnk_frontend *ui){
 if(!ui||!ui->game.loaded)return -1;
 if(ui->state==CNK_FRONTEND_RACE){
  if(cnk_game_pause(&ui->game,1))return -2;
  ui->state=CNK_FRONTEND_PAUSE;
 }else if(ui->state==CNK_FRONTEND_PAUSE){
  if(cnk_game_pause(&ui->game,0))return -2;
  ui->state=CNK_FRONTEND_RACE;
 }else return -3;
 return 0;
}
int cnk_frontend_refresh(cnk_frontend *ui){
 if(!ui||!ui->game.loaded)return -1;
 if(ui->state==CNK_FRONTEND_RACE &&
    cnk_start_sequence_racing(&ui->start_sequence)){
  cnk_game_hud hud={0};
  if(cnk_game_hud_read(&ui->game,&hud))return -2;
  if(hud.finished){
   if(ui->cup.active || ui->story.active){
    cnk_game_classification board={0};
    if(cnk_game_classification_read(&ui->game,&board))return -3;
    for(uint32_t i=0;i<board.count;i++)if(!board.rows[i].finished)return 0;
    const cnk_track_entry *track=cnk_frontend_selected(ui);
    if(!track)return -4;
    if(ui->cup.active && cnk_cup_commit(&ui->cup,track->resource_id,&board))return -4;
    if(ui->story.active){
     int success=cnk_story_complete(&ui->story,track->resource_id,&board);
     if(success<0)return -5;
     ui->story_last_attempt_success=success;
    }
   }
   ui->state=CNK_FRONTEND_RESULTS;
  }
 }
 return 0;
}
int cnk_frontend_update(cnk_frontend *ui,cnk_driver_input input,uint32_t dt_us){
 if(!ui||!ui->game.loaded)return -1;
 if(ui->state==CNK_FRONTEND_SELECT||ui->state==CNK_FRONTEND_PAUSE||
    ui->state==CNK_FRONTEND_RESULTS)return 0;
 if(ui->state!=CNK_FRONTEND_RACE)return -2;
 uint32_t driving_us=0,events=0;
 cnk_start_phase old_phase=ui->start_sequence.phase;
 uint32_t old_remaining=ui->start_sequence.remaining_us;
 if(cnk_start_sequence_step(&ui->start_sequence,dt_us,&driving_us,&events))return -3;
 int launch=cnk_launch_step(&ui->launch,old_phase,old_remaining,dt_us,
                            input.throttle,events);
 if(launch<0)return -4;
 if(launch && cnk_game_apply_launch_boost(&ui->game))return -5;
 ui->start_events|=events;
 if(!driving_us)return 0;
 int steps=cnk_game_update(&ui->game,input,driving_us);
 if(steps<0)return steps;
 if(cnk_frontend_refresh(ui))return -3;
 return steps;
}
int cnk_frontend_back_to_select(cnk_frontend *ui){
 if(!ui||ui->state==CNK_FRONTEND_EMPTY)return -1;
 cnk_game_unload(&ui->game);
 ui->state=CNK_FRONTEND_SELECT;
 ui->start_sequence=(cnk_start_sequence){0};
 cnk_launch_reset(&ui->launch);
 ui->start_events=0;
 ui->cup=(cnk_cup){0};
 ui->story=(cnk_story){0};
 ui->story_last_attempt_success=0;
 return 0;
}
int cnk_frontend_restart(cnk_frontend *ui){
 if(!ui||ui->state==CNK_FRONTEND_EMPTY)return -1;
 cnk_game_unload(&ui->game);
 ui->state=CNK_FRONTEND_SELECT;
 ui->start_sequence=(cnk_start_sequence){0};
 cnk_launch_reset(&ui->launch);
 ui->start_events=0;
 ui->story_last_attempt_success=0;
 return cnk_frontend_start(ui);
}

uint32_t cnk_frontend_take_start_events(cnk_frontend *ui){
 if(!ui)return 0;
 uint32_t events=ui->start_events;
 ui->start_events=0;
 return events;
}
