#include "cnk_app_runtime.h"
#include "cnk_projectile_threat.h"
#include <string.h>

int cnk_app_runtime_init(cnk_app_runtime *app,const fufs_file *vfs,
                         uint32_t racers,uint32_t laps){
 if(!app || !vfs)return -1;
 memset(app,0,sizeof(*app));
 if(cnk_frontend_init(&app->frontend,vfs,racers,laps))return -2;
 app->initialized=1;
 return 0;
}
int cnk_app_runtime_snapshot(const cnk_app_runtime *app,cnk_app_snapshot *out){
 if(!app||!out||!app->initialized)return -1;
 cnk_app_snapshot s={0};
 s.state=app->frontend.state;
 s.cup_available=app->frontend.cup_manifest_ready;
 s.story_available=app->frontend.story_manifest_ready;
 s.story_active=app->frontend.story.active;
 s.story_completed=app->frontend.story.completed;
 s.story_total=app->frontend.story.stage_count;
 s.story_last_attempt_success=app->frontend.story_last_attempt_success;
 s.story_write_error=app->story_write_error;
 if(app->completed_story_archive.active &&
    app->completed_story_archive.completed==app->completed_story_archive.stage_count){
  s.has_completed_story=1;
  s.completed_story_count=app->completed_story_archive.stage_count;
  memcpy(s.completed_story_ranks,app->completed_story_archive.achieved_ranks,
         sizeof(s.completed_story_ranks));
 }
 s.cup_write_error=app->cup_write_error;
 if(app->completed_cup_archive.active &&
    app->completed_cup_archive.completed_rounds==app->completed_cup_archive.round_count){
  if(cnk_cup_standings_read(&app->completed_cup_archive,
                            &s.completed_cup_standings))return -12;
  s.has_completed_cup=1;
 }
 s.selected_race=app->frontend.selected_race;
 s.practice_mode=app->frontend.practice_mode;
 s.configured_laps=app->frontend.total_laps;
 s.records_write_error=app->records_write_error;
 s.available_races=app->frontend.catalog.race_count;
 if(app->frontend.cup.active){
  if(cnk_cup_standings_read(&app->frontend.cup,&s.cup_standings))return -11;
  s.cup_active=1;
 }
 s.rendered_frames=app->rendered_frames;
 s.start_phase=app->frontend.start_sequence.phase;
 s.start_phase_remaining_ms=(app->frontend.start_sequence.remaining_us+999u)/1000u;
 const cnk_track_entry *entry=cnk_frontend_selected(&app->frontend);
 if(entry){
  s.selected_vfs_index=entry->vfs_index;
  s.source_resource_id=entry->resource_id;
  int rec=cnk_records_find(&app->records,entry->resource_id,
       app->frontend.total_laps,app->frontend.practice_mode?
       CNK_RECORDS_PRACTICE:CNK_RECORDS_NORMAL,&s.personal_best);
  if(rec<0)return -4;
  s.has_personal_best=rec==1;
 }
 if(app->frontend.game.loaded){
  if(cnk_game_hud_read(&app->frontend.game,&s.hud))return -2;
  if(cnk_game_classification_read(&app->frontend.game,&s.classification))return -3;
  /* Reuse cached source-route samples. Update only per-racer screen dots,
   * preserving the original geometry and frame performance on Vita. */
  s.minimap=app->frontend.game.route_minimap;
  if(s.minimap.path_count &&
     cnk_minimap_update_racers(&app->frontend.game.session,&s.minimap))
   s.minimap=(cnk_minimap_frame){0};
  s.player_boost=app->frontend.game.session.racers[0].boost;
  float required=app->frontend.game.session.boost_tuning.charge_seconds;
  if(required>0.0f){
   s.boost_charge_fraction=s.player_boost.charge_seconds/required;
   if(s.boost_charge_fraction>1.0f)s.boost_charge_fraction=1.0f;
  }
  s.boost_active=s.player_boost.boost_left_seconds>0.0f;
  s.player_items=app->frontend.game.session.items.racers[0];
  const cnk_item_world *items=&app->frontend.game.session.items;
  s.clock_seconds=items->clock_seconds;
  s.candle_seconds=items->candle_seconds;
  s.clock_affects_player=items->clock_seconds>0.0f && items->clock_owner!=0u;
  s.candle_affects_player=items->candle_seconds>0.0f && items->candle_owner!=0u;
  /* Display-only prediction. It never changes player steering, physics,
   * inventory or course progress. No historical Zeebo art/sound is assumed. */
  if(!app->frontend.game.session.racers[0].race.finished){
   cnk_projectile_threat threat={0};
   if(cnk_projectile_threat_read(&app->frontend.game.session.items,0,
            &app->frontend.game.session.racers[0].kart,&threat))return -5;
   if(threat.incoming){
    s.incoming_projectile=1;
    s.incoming_kind=threat.kind;
    s.incoming_eta_ms=(uint32_t)(threat.eta_seconds*1000.0f+0.5f);
    s.incoming_lateral_side=threat.lateral_side;
   }
  }
  s.placed_pickups=app->frontend.game.session.items.pickup_count;
  s.placed_pads=app->frontend.game.session.pads.count;
  for(unsigned j=0;j<CNK_ITEM_MAX_MISSILES;j++)
   if(app->frontend.game.session.items.missiles[j].active)s.active_missiles++;
  for(unsigned j=0;j<CNK_ITEM_MAX_HAZARDS;j++)
   if(app->frontend.game.session.items.hazards[j].active)s.active_hazards++;
  s.has_hud=1;
 }
 *out=s;
 return 0;
}
int cnk_app_runtime_tick(cnk_app_runtime *app,uint64_t now_us,
                         uint32_t held_ui_buttons,cnk_driver_input driving,
                         cnk_app_snapshot *snapshot){
 if(!app||!app->initialized||!snapshot)return -1;
 uint32_t delta=0;
 if(!app->clock_initialized){
  app->last_clock_us=now_us;
  app->clock_initialized=1;
 }else if(now_us>=app->last_clock_us){
  uint64_t gap=now_us-app->last_clock_us;
  delta=(uint32_t)(gap>250000u?250000u:gap);
  app->last_clock_us=now_us;
 }else if(app->last_clock_us-now_us>1000000u){
  /* A large clock reset (e.g. suspended process / clock source reset)
   * rebases without injecting elapsed negative time into race or countdown.
   * Small backwards jitter is ignored until the clock catches up. */
  app->last_clock_us=now_us;
 }
 app->last_delta_us=delta;
 cnk_frontend_state old_state=app->frontend.state;
 uint32_t old_track=app->frontend.selected_race;
 uint32_t old_starts=app->frontend.starts;
 uint32_t old_completed=app->frontend.cup.completed_rounds;
 uint32_t old_story_completed=app->frontend.story.completed;
 int old_practice_mode=app->frontend.practice_mode;
 uint32_t old_laps=app->frontend.total_laps;
 const uint32_t previous_recoveries=app->frontend.game.loaded ?
     app->frontend.game.session.racers[0].recovery_count : 0;
 const int was_wrong_way=app->frontend.game.loaded &&
     app->frontend.game.session.racers[0].course_direction.wrong_way;
 int rc=cnk_frontend_dispatch(&app->frontend,&app->input_edges,
                              held_ui_buttons,driving,delta);
 if(rc<0)return -2;
 if(app->frontend.starts!=old_starts && app->frontend.game.loaded){
  if(cnk_game_enable_provisional_walls(&app->frontend.game,app->collision_probe.walls)<0 ||
     cnk_game_enable_provisional_kart_contacts(&app->frontend.game,app->collision_probe.karts)<0)return -10;
 }
 if(app->item_layout && app->frontend.starts!=old_starts &&
    cnk_item_layout_apply(app->item_layout,&app->frontend.game)<0)return -8;
 if(app->pad_layout && app->frontend.starts!=old_starts &&
    cnk_pad_layout_apply(app->pad_layout,&app->frontend.game)<0)return -9;
 /* Only successful, source-backed race frames may yield local records.
  * Aborted/incomplete races do not get a finish time. An accepted LAP0
  * makes best_lap_ms nonzero; the race code ignores invalid checkpoints. */
 int record_changed=0;
 if(app->frontend.game.loaded && app->frontend.starts==old_starts &&
    old_state==CNK_FRONTEND_RACE &&
    (app->frontend.state==CNK_FRONTEND_RACE ||
     app->frontend.state==CNK_FRONTEND_RESULTS) &&
    app->frontend.game.session.racers[0].race.last_ms>0){
  const cnk_game *g=&app->frontend.game;
  const cnk_race *r=&g->session.racers[0].race;
  const cnk_track_entry *track=cnk_frontend_selected(&app->frontend);
  if(track && (r->best_lap_ms || r->finished)){
   uint64_t final_ms=0;
   if(r->finished && g->session.racers[0].finish_ms>=r->start_ms)
    final_ms=g->session.racers[0].finish_ms-r->start_ms;
   int rc=cnk_records_update(&app->records,track->resource_id,
     r->total_laps,app->frontend.practice_mode?CNK_RECORDS_PRACTICE:
     CNK_RECORDS_NORMAL,r->best_lap_ms,final_ms);
   if(rc<0){
    /* Record storage is best effort. Reaching capacity or an invalid
     * external record table must never kill a running race. */
    app->records_write_error=rc;
   }else record_changed=rc;
   if(record_changed && app->records_path[0])
    app->records_write_error=cnk_records_save(&app->records,app->records_path);
  }
 }
 /* A Cup round is durable only after ALL racers finish and the validated
  * positions have been committed. Never save a partial race in flight.
  * Failures are informational: storage must not end an otherwise valid race. */
 if(app->cup_checkpoint_path[0] && app->frontend.cup.active &&
    app->frontend.cup.completed_rounds>old_completed){
  app->cup_write_error=cnk_cup_checkpoint_save(&app->frontend.cup,
                                               app->cup_checkpoint_path);
  /* A completed and verified Cup remains reviewable even after returning
   * to the selector; never display a partly completed run as final. */
  if(app->frontend.cup.completed_rounds==app->frontend.cup.round_count &&
     app->cup_write_error==0)
   app->completed_cup_archive=app->frontend.cup;
 }
 /* Persist only stage objectives already achieved with a fully finished board.
  * A failure to write never interrupts a legitimate race. */
 if(app->story_checkpoint_path[0] && app->frontend.story.active &&
    app->frontend.story.completed>old_story_completed){
  app->story_write_error=cnk_story_checkpoint_save(&app->frontend.story,
                                                   app->story_checkpoint_path);
  if(!app->story_write_error &&
     app->frontend.story.completed==app->frontend.story.stage_count)
   app->completed_story_archive=app->frontend.story;
 }
 if(cnk_app_runtime_snapshot(app,snapshot))return -3;
 /* A single sound-routing edge at onset, not 60 repeated WAV triggers/sec.
  * No sample is assigned until a reviewed original WAV mapping exists. */
 int is_warning=app->frontend.state==CNK_FRONTEND_RACE &&
                snapshot->has_hud && snapshot->incoming_projectile;
 if(is_warning && !app->last_projectile_warning &&
    app->frontend.starts==old_starts)
  snapshot->app_events|=CNK_APP_EVENT_INCOMING_PROJECTILE;
 app->last_projectile_warning=is_warning;
 if(record_changed)snapshot->app_events|=CNK_APP_EVENT_NEW_RECORD;
 if(old_practice_mode!=app->frontend.practice_mode)
  snapshot->app_events|=CNK_APP_EVENT_MODE_CHANGED;
 if(old_laps!=app->frontend.total_laps)
  snapshot->app_events|=CNK_APP_EVENT_LAPS_CHANGED;
 /* App events are derived only from COMPLETED, successful transitions.
    A held UI button will not repeat the transition; those edges are handled
    by cnk_frontend_dispatch. Events are not accumulated across frames. */
 uint32_t start_events=cnk_frontend_take_start_events(&app->frontend);
 if(start_events&CNK_START_EVENT_3)snapshot->app_events|=CNK_APP_EVENT_COUNT_3;
 if(start_events&CNK_START_EVENT_2)snapshot->app_events|=CNK_APP_EVENT_COUNT_2;
 if(start_events&CNK_START_EVENT_1)snapshot->app_events|=CNK_APP_EVENT_COUNT_1;
 if(start_events&CNK_START_EVENT_GO)snapshot->app_events|=CNK_APP_EVENT_GO;
 if(old_track!=app->frontend.selected_race)
  snapshot->app_events|=CNK_APP_EVENT_TRACK_CHANGED;
 if(app->frontend.starts!=old_starts){
  snapshot->app_events|=CNK_APP_EVENT_RACE_STARTED;
  if(old_state!=CNK_FRONTEND_SELECT)
   snapshot->app_events|=CNK_APP_EVENT_RACE_RESTARTED;
 }
 if(old_state!=app->frontend.state){
  if(app->frontend.state==CNK_FRONTEND_PAUSE)
   snapshot->app_events|=CNK_APP_EVENT_PAUSED;
  if(old_state==CNK_FRONTEND_PAUSE && app->frontend.state==CNK_FRONTEND_RACE &&
     app->frontend.starts==old_starts)
   snapshot->app_events|=CNK_APP_EVENT_RESUMED;
  if(app->frontend.state==CNK_FRONTEND_SELECT)
   snapshot->app_events|=CNK_APP_EVENT_BACK_TO_SELECT;
  if(app->frontend.state==CNK_FRONTEND_RESULTS)
   snapshot->app_events|=CNK_APP_EVENT_RESULTS;
 }
 if(app->frontend.game.loaded){
  if(app->frontend.starts==old_starts &&
     app->frontend.game.session.racers[0].recovery_count>previous_recoveries)
    snapshot->app_events|=CNK_APP_EVENT_KART_RECOVERED;
  /* Transitions only after a successful racing simulation frame; never
   * trigger a spurious cue on load, restart, pause or scene selection. */
  if(app->frontend.starts==old_starts &&
     old_state==CNK_FRONTEND_RACE && app->frontend.state==CNK_FRONTEND_RACE){
    int wrong=snapshot->hud.wrong_way;
    if(!was_wrong_way && wrong)snapshot->app_events|=CNK_APP_EVENT_WRONG_WAY_START;
    if(was_wrong_way && !wrong)snapshot->app_events|=CNK_APP_EVENT_WRONG_WAY_CLEAR;
  }
  uint32_t count=app->frontend.game.session.racer_count;
  if(count>CNK_MAX_RACERS)return -4;
  uint32_t boost_events=0;
  if(cnk_game_take_boost_events(&app->frontend.game,&boost_events))return -6;
  if(boost_events&CNK_BOOST_DRIFT_STARTED)snapshot->app_events|=CNK_APP_EVENT_DRIFT_START;
  if(boost_events&CNK_BOOST_DRIFT_STOPPED)snapshot->app_events|=CNK_APP_EVENT_DRIFT_STOP;
  if(boost_events&CNK_BOOST_ACTIVATED)snapshot->app_events|=CNK_APP_EVENT_BOOST_START;
  if(boost_events&CNK_BOOST_LAUNCH)snapshot->app_events|=CNK_APP_EVENT_LAUNCH_BOOST;
  if(boost_events&CNK_BOOST_EXPIRED)snapshot->app_events|=CNK_APP_EVENT_BOOST_END;
  for(uint32_t i=0;i<count;i++){
   if(cnk_game_take_events(&app->frontend.game,i,&snapshot->race_events[i]))return -5;
   if(cnk_game_take_item_events(&app->frontend.game,i,&snapshot->item_events[i]))return -7;
  }
  uint32_t items=snapshot->item_events[0];
  if(items&CNK_ITEM_EVENT_WEAPON)snapshot->app_events|=CNK_APP_EVENT_ITEM_COLLECTED;
  if(items&CNK_ITEM_EVENT_WUMPA)snapshot->app_events|=CNK_APP_EVENT_WUMPA_COLLECTED;
  if(items&CNK_ITEM_EVENT_USED)snapshot->app_events|=CNK_APP_EVENT_ITEM_FIRED;
  if(items&CNK_ITEM_EVENT_HIT)snapshot->app_events|=CNK_APP_EVENT_ITEM_HIT;
  if(items&CNK_ITEM_EVENT_MISSILE_FIRED)snapshot->app_events|=CNK_APP_EVENT_MISSILE_FIRED;
  if(items&CNK_ITEM_EVENT_HAZARD_DROPPED)snapshot->app_events|=CNK_APP_EVENT_HAZARD_DROPPED;
  if(items&CNK_ITEM_EVENT_MISSILE_HIT)snapshot->app_events|=CNK_APP_EVENT_MISSILE_HIT;
  if(items&CNK_ITEM_EVENT_TNT_HIT)snapshot->app_events|=CNK_APP_EVENT_TNT_HIT;
  if(items&CNK_ITEM_EVENT_OIL_HIT)snapshot->app_events|=CNK_APP_EVENT_OIL_HIT;
 }
 snapshot->simulated_steps=rc;
 app->rendered_frames++;
 snapshot->rendered_frames=app->rendered_frames;
 return 0;
}
void cnk_app_runtime_close(cnk_app_runtime *app){
 if(!app)return;
 if(app->initialized)cnk_frontend_close(&app->frontend);
 memset(app,0,sizeof(*app));
}

int cnk_app_runtime_set_item_layout(cnk_app_runtime *app,const cnk_item_layout *layout){
 if(!app||!app->initialized||app->frontend.game.loaded)return -1;
 app->item_layout=layout;
 return 0;
}

int cnk_app_runtime_set_records_path(cnk_app_runtime *app,const char *path){
 if(!app||!app->initialized||!path||!path[0]||
    app->frontend.game.loaded||strlen(path)>=CNK_RECORDS_PATH_LIMIT-4u)return -1;
 cnk_records incoming={0};
 int rc=cnk_records_load(&incoming,path);
 if(rc && cnk_records_load_backup(&incoming,path)==0){
  /* A torn/corrupt primary can be recovered from the verified previous save.
   * Keep the backup untouched until a later successful save repairs primary. */
  rc=0;
 }
 if(rc && rc!=-2)return -2; /* no valid primary OR backup: never erase */
 if(!rc)app->records=incoming;
 memcpy(app->records_path,path,strlen(path)+1u);
 app->records_write_error=0;
 return 0;
}

int cnk_app_runtime_set_cup_checkpoint_path(cnk_app_runtime *app,const char *path){
 if(!app||!app->initialized||!path||!path[0]||
    strlen(path)>CNK_CUP_CHECKPOINT_PATH_MAX-5u ||
    !app->frontend.cup_manifest_ready||app->frontend.game.loaded||
    app->frontend.state!=CNK_FRONTEND_SELECT)return -1;
 cnk_cup candidate={0};
 int rc=cnk_cup_checkpoint_load(&candidate,path,&app->frontend.cup_manifest,
                                 &app->frontend.catalog);
 if(rc){
  /* Recover only a second independently authenticated snapshot. If neither
   * file is valid, keep the damaged primary intact and disable writes. */
  int backup_rc=cnk_cup_checkpoint_load_backup(&candidate,path,
                              &app->frontend.cup_manifest,&app->frontend.catalog);
  if(!backup_rc)rc=0;
  else if(rc!=-2)return -2; /* do not silently overwrite a damaged save */
 }
 if(!rc){
  if(candidate.completed_rounds<candidate.round_count){
   if(cnk_frontend_cup_resume(&app->frontend,&candidate))return -3;
  }else{
   /* A finished verified Cup is not a race to resume. Preserve its final
    * table for the SELECT screen without blocking new Cup configuration. */
   app->completed_cup_archive=candidate;
  }
 }
 memcpy(app->cup_checkpoint_path,path,strlen(path)+1u);
 app->cup_write_error=0;
 return 0;
}

int cnk_app_runtime_set_story_checkpoint_path(cnk_app_runtime *app,const char *path){
 if(!app||!app->initialized||!path||!path[0]||
    strlen(path)>CNK_STORY_CHECKPOINT_PATH_MAX-5u||
    !app->frontend.story_manifest_ready||app->frontend.game.loaded||
    app->frontend.state!=CNK_FRONTEND_SELECT)return -1;
 cnk_story candidate={0};
 int rc=cnk_story_checkpoint_load(&candidate,path,&app->frontend.story_manifest,
               &app->frontend.catalog,app->frontend.racer_count,0);
 if(rc){
  int backup_rc=cnk_story_checkpoint_load_backup(&candidate,path,
               &app->frontend.story_manifest,&app->frontend.catalog,
               app->frontend.racer_count,0);
  if(!backup_rc)rc=0;
  else if(rc!=-2)return -2; /* leave damaged save untouched */
 }
 if(!rc){
  if(candidate.completed<candidate.stage_count){
   if(cnk_frontend_story_resume(&app->frontend,&candidate))return -3;
  }else app->completed_story_archive=candidate;
 }
 memcpy(app->story_checkpoint_path,path,strlen(path)+1u);
 app->story_write_error=0;
 return 0;
}

int cnk_app_runtime_set_pad_layout(cnk_app_runtime *app,const cnk_pad_layout *layout){
 if(!app||!app->initialized||app->frontend.game.loaded||
    !layout||layout->count>CNK_PAD_LAYOUT_MAX)return -1;
 /* Read-only sidecar: the runtime never owns or alters the caller's entries.
  * Validate all unique source-id groups before installing the mapping. */
 for(uint32_t i=0;i<layout->count;i++){
  cnk_boost_pad_world check={0};
  uint32_t id=layout->entries[i].scene_resource_id;
  if(!id)return -2;
  for(uint32_t j=0;j<layout->count;j++)if(layout->entries[j].scene_resource_id==id){
   if(cnk_boost_pad_place(&check,layout->entries[j].position,
                          layout->entries[j].radius))return -2;
  }
 }
 app->pad_layout=layout;
 return 0;
}

int cnk_app_runtime_set_collision_probe(cnk_app_runtime *app,cnk_collision_probe mode){
 if(!app||!app->initialized||app->frontend.state!=CNK_FRONTEND_SELECT||
    (mode.walls!=0&&mode.walls!=1)||(mode.karts!=0&&mode.karts!=1))return -1;
 app->collision_probe=mode;
 return 0;
}
