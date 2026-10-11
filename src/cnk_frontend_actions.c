#include "cnk_frontend_actions.h"

int cnk_frontend_dispatch(cnk_frontend *ui,cnk_frontend_action_state *state,
                          uint32_t held,cnk_driver_input driving,uint32_t dt_us){
 if(!ui||!state||ui->state==CNK_FRONTEND_EMPTY||
    (held&~(uint32_t)CNK_UI_ALL))return -1;
 uint32_t pressed=held&~state->last_buttons;
 state->last_buttons=held;
 switch(ui->state){
 case CNK_FRONTEND_SELECT:
  if(pressed&CNK_UI_PAUSE){
   if(ui->practice_mode)return 0;
   if(ui->story_manifest_ready)return cnk_frontend_story_begin(ui,
     ui->story_manifest.stages,ui->story_manifest.stage_count);
   if(!ui->cup_manifest_ready)return 0;
   return cnk_frontend_cup_begin(ui,ui->cup_manifest.resource_ids,
           ui->cup_manifest.round_count,ui->cup_manifest.points);
  }
  if(pressed&CNK_UI_LAPS)return cnk_frontend_cycle_laps(ui);
  if(pressed&CNK_UI_RESTART)return cnk_frontend_toggle_practice(ui);
  if(pressed&CNK_UI_CONFIRM)return cnk_frontend_start(ui);
  if(pressed&CNK_UI_PREVIOUS)return cnk_frontend_select(ui,-1);
  if(pressed&CNK_UI_NEXT)return cnk_frontend_select(ui,1);
  return 0;
 case CNK_FRONTEND_RACE:
  if(pressed&CNK_UI_PAUSE)return cnk_frontend_toggle_pause(ui);
  if((pressed&(CNK_UI_RECOVER|CNK_UI_CANCEL))&&cnk_start_sequence_racing(&ui->start_sequence)){
   /* Race recovery is allowed only after GO and before the finish. */
   if(!ui->game.session.racers[0].race.finished &&
      cnk_game_recover_player(&ui->game))return -5;
  }
  if(cnk_game_set_player_drift(&ui->game,!!(held&CNK_UI_DRIFT)))return -3;
  /* Never buffer item use during 3-2-1: an early L press cannot fire at GO. */
  if((pressed&CNK_UI_ITEM) && cnk_start_sequence_racing(&ui->start_sequence))
   if(cnk_game_request_item(&ui->game))return -4;
  return cnk_frontend_update(ui,driving,dt_us);
 case CNK_FRONTEND_PAUSE:
  if(pressed&CNK_UI_CANCEL)return cnk_frontend_back_to_select(ui);
  if(pressed&CNK_UI_RESTART)return cnk_frontend_restart(ui);
  if(pressed&(CNK_UI_PAUSE|CNK_UI_CONFIRM))return cnk_frontend_toggle_pause(ui);
  return 0;
 case CNK_FRONTEND_RESULTS:
  if(pressed&CNK_UI_CANCEL)return cnk_frontend_back_to_select(ui);
  if(pressed&(CNK_UI_CONFIRM|CNK_UI_RESTART)){
   if(ui->cup.active){int rc=cnk_frontend_cup_next(ui);return rc>=0?0:rc;}
   if(ui->story.active){
    if(ui->story_last_attempt_success){int rc=cnk_frontend_story_next(ui);return rc>=0?0:rc;}
    return cnk_frontend_restart(ui);
   }
   return cnk_frontend_restart(ui);
  }
  return 0;
 default:return -2;
 }
}
