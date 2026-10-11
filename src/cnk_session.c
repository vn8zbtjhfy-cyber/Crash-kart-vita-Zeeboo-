#include "cnk_session.h"
#include "cnk_race_progress.h"
#include "cnk_ai_traffic.h"
#include <math.h>
#include <string.h>
#include <limits.h>
static int score_higher(const cnk_session *s,uint32_t a,uint32_t b){
 const cnk_session_racer *x=&s->racers[a],*y=&s->racers[b];
 if(x->race.finished!=y->race.finished)return x->race.finished>y->race.finished;
 if(x->race.finished){
  if(x->finish_ms!=y->finish_ms)return x->finish_ms<y->finish_ms;
  return a<b;
 }
 if(x->race.completed_laps!=y->race.completed_laps)
  return x->race.completed_laps>y->race.completed_laps;
 /* The true opponent grid is unknown: preserve the deterministic initial
  * slot ordering until the first race simulation frame (not a guessed
  * result based on relative proximity of arbitrary test-grid positions). */
 if(x->race.last_ms==x->race.start_ms &&
    y->race.last_ms==y->race.start_ms)return a<b;
 cnk_race_progress px={0},py={0};
 int valid_x=!cnk_race_progress_read(&x->race,x->route_fraction,&px);
 int valid_y=!cnk_race_progress_read(&y->race,y->route_fraction,&py);
 /* Only a crossed source LAP* gate advances the primary race rank.
  * This prevents a missed checkpoint or route projection onto a distant
  * part of the track from overtaking a legitimate checkpoint finisher. */
 if(valid_x!=valid_y)return valid_x>valid_y;
 if(valid_x){
  if(px.passed_gates!=py.passed_gates)
   return px.passed_gates>py.passed_gates;
  if(fabsf(px.sector_fraction-py.sector_fraction)>0.0001f)
   return px.sector_fraction>py.sector_fraction;
 }
 return a<b;
}
static void sort_standings(cnk_session *s){
 for(uint32_t i=0;i<s->racer_count;i++)s->standings[i]=i;
 for(uint32_t i=1;i<s->racer_count;i++){
  uint32_t x=s->standings[i],j=i;
  while(j>0&&score_higher(s,x,s->standings[j-1])){
   s->standings[j]=s->standings[j-1];j--;
  }
  s->standings[j]=x;
 }
}
/* Recovery positions are source-route-derived only. Stop well before the
 * following gate, even in short source-route sectors. */
static int recovery_pose_at_gate(const cnk_session *s,uint32_t gate,cnk_kart *pose){
 if(!s||!pose||!s->route||gate>=s->track.gate_count)return -1;
 uint32_t next=(gate+1u)%s->track.gate_count;
 float f=s->track.gates[gate].fraction;
 float d=s->track.gates[next].fraction-f;
 if(d<=0.0f)d+=1.0f;
 if(!isfinite(d)||d<=0.000001f)return -2;
 float offset=fminf(0.002f,d*0.20f);
 cnk_vec3 a={0},b={0};
 if(cnk_nav_sample(s->route,f+offset,1,&a)||
    cnk_nav_sample(s->route,f+offset+fminf(0.001f,d*0.10f),1,&b))return -3;
 if(!isfinite(a.x)||!isfinite(a.y)||!isfinite(a.z)||
    !isfinite(b.x)||!isfinite(b.z))return -4;
 float dx=b.x-a.x,dz=b.z-a.z;
 if(dx*dx+dz*dz<0.000001f)return -5;
 return cnk_kart_spawn(pose,a,atan2f(dx,dz));
}
static void record_recovery_gate(cnk_session *s,cnk_session_racer *r){
 uint32_t n=s->track.gate_count;
 if(!n)return;
 uint32_t gate=r->race.last_accepted_gate;
 /* A later out-of-order crossing in the same physics frame invalidates
  * the saved checkpoint: do not resurrect an already rejected shortcut. */
 if(gate>=n || r->race.next_gate!=(gate+1u)%n)return;
 cnk_kart pose={0};
 if(!recovery_pose_at_gate(s,gate,&pose)){
  r->recovery_kart=pose;
  r->recovery_gate=gate;
 }
}
static int start_with_mode(cnk_session *s,const cnk_scene *scene,
                           uint32_t total_laps,uint32_t racers,uint64_t now_ms,
                           int original_player_spawn){
 if(!s)return -1;
 memset(s,0,sizeof(*s));
 if(!scene||racers<1||racers>CNK_MAX_RACERS||!total_laps||total_laps>999)return -2;
 const cnk_sline *route=cnk_scene_line(scene,"DRV0");
 if(!route||!route->points||route->count<3)return -3;
 cnk_race_track track={0};
 if(cnk_race_track_build(scene,&track))return -4;
 s->track=track;s->source=scene;s->route=route;s->racer_count=racers;s->now_ms=now_ms;
 cnk_drive_default_tuning(&s->tuning);
 cnk_boost_default(&s->boost_tuning);
 cnk_world_collision_default(&s->world_tuning);
 cnk_kart_contact_default(&s->contact_tuning);
 float total=cnk_nav_length(route,1);
 if(!isfinite(total)||total<1)return -5;
 /* Source STRT is the exact Zeebo launch marker. It belongs to the
  * human kart only for now; its opponent grid arrangement is not decoded.
  * Validated route starts for the other racers prevent attributing fabricated
  * positions to the ROM. */
 cnk_vec3 source_start={0};float source_heading=0;
 if(original_player_spawn &&
    cnk_scene_start_pose(scene,&source_start,&source_heading)){
  memset(s,0,sizeof(*s));return -8;
 }
 /* The original opponent starting grid is not present in the recovered
  * metadata. In source-spawn mode, place DEVELOPMENT AI karts at distinct
  * locations BEHIND the verified STRT marker on the original DRV0 polyline.
  * This avoids the old 0.20-unit offsets, which placed six karts almost on
  * top of one another and made experimental contact physics unusable.
  * Single-file, 7-unit separation is NOT asserted to be Zeebo's real grid. */
 cnk_nav_projection launch_projection={0};
 if(original_player_spawn &&
    cnk_nav_nearest(route,source_start,1,&launch_projection)){
  memset(s,0,sizeof(*s));return -9;
 }
 for(uint32_t i=0;i<racers;i++){
  cnk_session_racer *r=&s->racers[i];
  float fraction=s->track.gates[0].fraction+0.004f;
  if(original_player_spawn){
   fraction=launch_projection.fraction-(7.0f*(float)i)/total;
  }
  cnk_vec3 position,ahead;
  float tangent_step=original_player_spawn?0.25f/total:0.002f;
  if(cnk_nav_sample(route,fraction,1,&position)||
     cnk_nav_sample(route,fraction+tangent_step,1,&ahead)){
   memset(s,0,sizeof(*s));return -6;
  }
  float heading=atan2f(ahead.x-position.x,ahead.z-position.z);
  /* Keep the preexisting generic test-grid mode unchanged; it is not used
   * by native source catalog race launches. */
  if(!original_player_spawn){
   position.x+=cosf(heading)*(float)i*0.20f;
   position.z-=sinf(heading)*(float)i*0.20f;
  }else if(i==0){
   position=source_start;heading=source_heading;
  }
  if(cnk_kart_spawn(&r->kart,position,heading)||
     cnk_race_start(&r->race,&s->track,total_laps,now_ms)||
     cnk_ai_init(&r->ai,route,1,&s->tuning)){
   memset(s,0,sizeof(*s));return -6;
  }
  r->route_fraction=fraction-floorf(fraction);
  if(original_player_spawn && i==0)r->route_fraction=launch_projection.fraction;
  r->recovery_kart=r->kart; /* original STRT for human source-spawn mode */
 }
 if(cnk_camera_follow(&s->camera,&s->racers[0].kart,9,5,6,1.2f,960.0f/544.0f)){
  memset(s,0,sizeof(*s));return -7;
 }
 sort_standings(s);s->active=1;return 0;
}
int cnk_session_start(cnk_session *s,const cnk_scene *scene,
                      uint32_t total_laps,uint32_t racers,uint64_t now_ms){
 return start_with_mode(s,scene,total_laps,racers,now_ms,0);
}
int cnk_session_start_source(cnk_session *s,const cnk_scene *scene,
                             uint32_t total_laps,uint32_t racers,uint64_t now_ms){
 return start_with_mode(s,scene,total_laps,racers,now_ms,1);
}
static void prime_recovered_gate_sides(cnk_session *s,
                                       cnk_session_racer *r,cnk_vec3 position);
/* Rivals are the only automatic users of the anti-stall watchdog. A rescue
 * cannot award checkpoints or alter a player's input/camera/inventory. */
static int recover_stalled_rival(cnk_session *s,uint32_t slot){
 if(!s||slot==0||slot>=s->racer_count)return -1;
 cnk_session_racer *r=&s->racers[slot];
 if(r->race.finished||r->recovery_count==UINT32_MAX)return -2;
 cnk_kart restored=r->recovery_kart;
 if(!isfinite(restored.position.x)||!isfinite(restored.position.y)||
    !isfinite(restored.position.z)||!isfinite(restored.heading))return -3;
 cnk_nav_projection projection={0};
 if(cnk_nav_nearest(s->route,restored.position,1,&projection))return -4;
 restored.speed=0;restored.steering=0;restored.ticks=r->kart.ticks;
 r->kart=restored;
 r->route_fraction=projection.fraction;
 r->ai.progress_initialized=0; /* reacquire its original source route */
 r->last_event=0;
 r->last_wall_contact=0;r->last_kart_contact=0;
 prime_recovered_gate_sides(s,r,restored.position);
 r->recovery_count++;
 return 0;
}
int cnk_session_step(cnk_session *s,cnk_driver_input input,float dt,
                     uint64_t now_ms,const cnk_collision_grid *surface){
 if(!s||!s->active||s->racer_count<1||now_ms<s->now_ms||
    !isfinite(dt)||dt<=0||dt>0.10f||
    !isfinite(input.throttle)||!isfinite(input.brake)||!isfinite(input.steer))return -1;
 cnk_driver_input controls[CNK_MAX_RACERS]={0};controls[0]=input;
 cnk_item_world next_items=s->items;
 uint32_t item_events[CNK_ITEM_MAX_RACERS]={0};
 cnk_vec3 item_before[CNK_ITEM_MAX_RACERS]={0};
 uint8_t item_movable[CNK_ITEM_MAX_RACERS]={0};
 for(uint32_t i=0;i<s->racer_count;i++){
  item_before[i]=s->racers[i].kart.position;
  item_movable[i]=!s->racers[i].race.finished;
 }
 /* Kart Jacking is delayed until the regular physics/item step succeeds.
  * Teleporting before the gate step could incorrectly register a shortcut. */
 int pending_jack=s->player_item_requested && item_movable[0] &&
     next_items.racers[0].held==CNK_WEAPON_KART_JACKING;
 if(s->player_item_requested && item_movable[0] && !pending_jack){
  if(cnk_items_use(&next_items,0,&s->racers[0].kart,&item_events[0])<0)return -7;
 }
 cnk_kart ai_item_karts[CNK_ITEM_MAX_RACERS]={0};
 for(uint32_t i=0;i<s->racer_count;i++)ai_item_karts[i]=s->racers[i].kart;
 for(uint32_t i=1;i<s->racer_count;i++)
  if(cnk_items_ai_use(&next_items,i,ai_item_karts,item_movable,
                       s->racer_count,&item_events[i])<0)return -7;
 for(uint32_t i=1;i<s->racer_count;i++){
  cnk_session_racer *r=&s->racers[i];
  r->last_ai_tactic=0;
  if(!r->race.finished){
   if(cnk_ai_controls(&r->ai,&r->kart,&s->tuning,&controls[i]))return -2;
   /* Provisional cursed-candle response: route following stays intact,
    * but tactical item/hazard recognition is suppressed for a blinded AI.
    * Source text proves visual darkness, not this exact AI policy. */
   if(!(next_items.candle_seconds>0.0f && next_items.candle_owner!=i) &&
      cnk_ai_tactics_apply(&next_items,s->route,i,&r->kart,&controls[i],
                           &r->last_ai_tactic))return -12;
   /* Items and moving projectiles outrank slower traffic. Reuse the
    * immutable pre-physics position/movable snapshots already captured for
    * item AI. This avoids copying all six karts separately for each rival
    * every 60 Hz frame, without changing contact priority or AI inputs. */
   if(!r->last_ai_tactic &&
      cnk_ai_traffic_apply(s->route,ai_item_karts,item_movable,
          s->racer_count,i,&controls[i],&r->last_ai_tactic))return -13;
  }
 }
 /* Compute optional human drift/boost tuning exactly once per 60Hz step.
  * Advance player boost state only after a successful kart integration. */
 cnk_drive_tuning player_tuning=s->tuning;
 cnk_boost_state next_boost=s->racers[0].boost;
 uint32_t player_boost_events=0;
 if(!s->racers[0].race.finished &&
    cnk_boost_step(&next_boost,&s->boost_tuning,&s->tuning,
       s->player_drift_held,controls[0].steer,s->racers[0].kart.speed,dt,
       &player_tuning,&player_boost_events))return -6;
 cnk_drive_tuning item_tunings[CNK_MAX_RACERS]={0};
 for(uint32_t i=0;i<s->racer_count;i++){
  if(cnk_items_tune(&next_items,i,i==0?&player_tuning:&s->tuning,
                    &item_tunings[i]))return -8;
  if(next_items.racers[i].stun_seconds>0){
   controls[i].throttle=0;
   controls[i].brake=1;
  }
 }
 int new_finishes=0;
 if(s->provisional_kart_contacts_enabled){
  /* All participants move first, then contacts are resolved, THEN gates are
   * advanced using final corrected positions. This prevents a rejected
   * movement through another kart from generating a false lap event. */
  cnk_vec3 before[CNK_MAX_RACERS]={0};
  cnk_kart candidates[CNK_MAX_RACERS]={0};
  uint8_t movable[CNK_MAX_RACERS]={0},contact_flags[CNK_MAX_RACERS]={0};
  int walls[CNK_MAX_RACERS]={0};
  for(uint32_t i=0;i<s->racer_count;i++){
   cnk_session_racer *r=&s->racers[i];
   before[i]=r->kart.position;
   candidates[i]=r->kart;
   movable[i]=!r->race.finished;
   if(!movable[i])continue;
   if(s->provisional_walls_enabled && surface){
    const cnk_drive_tuning *current=&item_tunings[i];
    int rc=cnk_drive_step_world(&candidates[i],current,controls[i],dt,
                                surface,&s->world_tuning);
    if(rc<0)return -3;
    walls[i]=rc==1;
   }else if(cnk_drive_step(&candidates[i],&item_tunings[i],
                          controls[i],dt,surface))return -3;
  }
  /* In this development race, a finisher's frozen final pose must not
   * become a permanent invisible finish-line barrier.  Run the optional
   * experimental contact solver only over karts still racing.  The resolver
   * itself continues to support static obstacles for its isolated tests;
   * this session policy is NOT a claim about Zeebo finish animations. */
  cnk_kart active_candidates[CNK_MAX_RACERS]={0};
  cnk_vec3 active_before[CNK_MAX_RACERS]={0};
  uint8_t active_movable[CNK_MAX_RACERS]={0};
  uint8_t active_flags[CNK_MAX_RACERS]={0};
  uint32_t active_slots[CNK_MAX_RACERS]={0},active_count=0;
  for(uint32_t i=0;i<s->racer_count;i++)if(movable[i]){
   active_slots[active_count]=i;
   active_candidates[active_count]=candidates[i];
   active_before[active_count]=before[i];
   active_movable[active_count]=1;
   active_count++;
  }
  if(active_count && cnk_kart_contacts_resolve(active_candidates,active_before,
       active_movable,active_count,&s->contact_tuning,active_flags)<0)return -5;
  for(uint32_t i=0;i<active_count;i++){
   candidates[active_slots[i]]=active_candidates[i];
   contact_flags[active_slots[i]]=active_flags[i];
  }
  for(uint32_t i=0;i<s->racer_count;i++){
   cnk_session_racer *r=&s->racers[i];
   r->kart=candidates[i];
   r->last_wall_contact=walls[i];
   r->last_kart_contact=contact_flags[i];
   if(walls[i]&&r->wall_contact_count<UINT64_MAX)r->wall_contact_count++;
   if(contact_flags[i]&&r->kart_contact_count<UINT64_MAX)r->kart_contact_count++;
   if(!movable[i]){r->last_event=0;r->last_boost_event=0;continue;}
   if(i==0){r->boost=next_boost;r->last_boost_event=player_boost_events;}
   int evt=cnk_race_advance(&r->race,before[i],r->kart.position,now_ms);
   if(evt<0)return -3;
   r->last_event=(uint32_t)evt;
   if(evt&CNK_RACE_FINISH){r->finish_ms=r->race.finish_ms;new_finishes++;}
   if((evt&CNK_RACE_GATE)&&!(evt&CNK_RACE_FINISH))record_recovery_gate(s,r);
   cnk_nav_projection nearest;
   if(!cnk_nav_nearest(s->route,r->kart.position,1,&nearest))
    r->route_fraction=nearest.fraction;
  }
 }else{
  /* Original v21 path completely preserved while experimental kart contacts
   * are disabled. The legacy race regression tests cover this branch. */
  for(uint32_t i=0;i<s->racer_count;i++){
   cnk_session_racer *r=&s->racers[i];
   r->last_kart_contact=0;
   if(r->race.finished){r->last_event=0;r->last_boost_event=0;r->last_wall_contact=0;continue;}
   int wall_contact=0;
   int evt=s->provisional_walls_enabled && surface ?
    cnk_drive_race_step_world_report(&r->kart,&item_tunings[i],controls[i],dt,surface,
        &s->world_tuning,&r->race,now_ms,&wall_contact) :
    cnk_drive_race_step(&r->kart,&item_tunings[i],controls[i],dt,surface,&r->race,now_ms);
   if(evt<0)return -3;
   if(i==0){r->boost=next_boost;r->last_boost_event=player_boost_events;}
   r->last_wall_contact=wall_contact;
   if(wall_contact && r->wall_contact_count<UINT64_MAX)r->wall_contact_count++;
   r->last_event=(uint32_t)evt;
   if(evt&CNK_RACE_FINISH){r->finish_ms=r->race.finish_ms;new_finishes++;}
   if((evt&CNK_RACE_GATE)&&!(evt&CNK_RACE_FINISH))record_recovery_gate(s,r);
   cnk_nav_projection nearest;
   if(!cnk_nav_nearest(s->route,r->kart.position,1,&nearest)){
    r->route_fraction=nearest.fraction;
   }
  }
 }
 /* Advisory only: source-path wrong-way warning on the human kart.
  * Calculated from final, wall/contact-resolved positions; no impact on
  * driving, race flags, opponent karts, ranking or checkpoints. */
 if(!s->racers[0].race.finished &&
    cnk_route_direction_step(&s->racers[0].course_direction,s->route,
      item_before[0],s->racers[0].kart.position,dt)){
  /* Advisory failures must NEVER abort a valid race or corrupt lap timing. */
  s->racers[0].course_direction=(cnk_route_direction){0};
 }
 /* Apply the item system to the actual race karts after their collision
  * resolution. Keep unverified placements absent until authored/validated. */
 cnk_kart item_karts[CNK_ITEM_MAX_RACERS]={0};
 for(uint32_t i=0;i<s->racer_count;i++)item_karts[i]=s->racers[i].kart;
 int item_rc=s->provisional_walls_enabled && surface ?
  cnk_items_step_world(&next_items,item_karts,item_before,item_movable,
     s->racer_count,dt,item_events,surface,s->world_tuning.max_abs_normal_y) :
  cnk_items_step(&next_items,item_karts,item_before,item_movable,
     s->racer_count,dt,item_events);
 if(item_rc)return -9;
 /* Source-reviewed pad entry is an independent timed speed impulse.
  * Staged alongside item effects to avoid partial inventory changes. */
 uint32_t pad_events[CNK_PAD_RACERS]={0};
 cnk_boost_pad_world next_pads=s->pads;
 if(cnk_boost_pad_step(&next_pads,&next_items,item_before,item_karts,
                      item_movable,s->racer_count,dt,pad_events))return -14;
 for(uint32_t i=0;i<s->racer_count;i++)if(pad_events[i]&CNK_PAD_EVENT_APPLIED)
  item_events[i]|=CNK_ITEM_EVENT_PAD_BOOST;
 /* Source-authored Kart Jacking exchanges poses only if both racers are
  * in precisely the same validated LAP/gate sector. Do not advance any race
  * through a teleport, or allow a finish to be awarded as a side effect. */
 if(pending_jack){
  uint8_t eligible[CNK_ITEM_MAX_RACERS]={0};
  for(uint32_t i=0;i<s->racer_count;i++)
   eligible[i]=!s->racers[i].race.finished &&
      s->racers[i].race.completed_laps==s->racers[0].race.completed_laps &&
      s->racers[i].race.next_gate==s->racers[0].race.next_gate;
  int swapped=cnk_items_try_jack(&next_items,0,item_karts,eligible,
                                  s->racer_count,item_events);
  if(swapped<0)return -15;
  if(swapped>0){
   uint32_t target=(uint32_t)swapped-1;
   for(uint32_t n=0;n<2;n++){
    uint32_t i=n==0?0:target;
    cnk_session_racer *r=&s->racers[i];
    r->kart.position=item_karts[i].position;
    r->kart.heading=item_karts[i].heading;
    r->kart.steering=0;
    prime_recovered_gate_sides(s,r,r->kart.position);
    cnk_nav_projection proj={0};
    if(!cnk_nav_nearest(s->route,r->kart.position,1,&proj))
     r->route_fraction=proj.fraction;
    r->course_direction=(cnk_route_direction){0};
    if(i!=0)r->ai.progress_initialized=0;
   }
  }
 }
 s->pads=next_pads;
 for(uint32_t i=0;i<s->racer_count;i++){
  s->racers[i].kart.speed=item_karts[i].speed;
  s->racers[i].last_item_event=item_events[i];
 }
 /* Anti-stall recovery is intentionally restricted to rival karts. It
  * observes resolved final positions, only during unpaused post-GO steps.
  * Damage/stun does not count as failed navigation. No generated gate events. */
 for(uint32_t i=1;i<s->racer_count;i++){
  cnk_session_racer *r=&s->racers[i];
  int able=!r->race.finished && next_items.racers[i].stun_seconds<=0.0f;
  int stalled=cnk_ai_watchdog_step(&r->ai_watchdog,r->kart.position,dt,able);
  if(stalled<0)return -10;
  if(stalled==1){
   if(recover_stalled_rival(s,i))return -11;
   r->ai_watchdog.anchor=r->kart.position;
  }
 }
 s->items=next_items;
 s->player_item_requested=0;
 s->now_ms=now_ms;
 if(cnk_camera_follow_obstructed(&s->camera,&s->racers[0].kart,
       surface,9,5,6,1.2f,960.0f/544.0f,0.4f))return -4;
 sort_standings(s);
 return new_finishes;
}
/* A teleport must NOT be fed through the gate advancement function.
 * Re-prime gate sides on the landing pose to suppress ghost checkpoint hits. */
static void prime_recovered_gate_sides(cnk_session *s,cnk_session_racer *r,
                                       cnk_vec3 position){
 for(uint32_t i=0;i<s->track.gate_count;i++){
  const cnk_race_gate *g=&s->track.gates[i];
  float cross=(g->b.x-g->a.x)*(position.z-g->a.z)-
              (g->b.z-g->a.z)*(position.x-g->a.x);
  r->race.last_side[i]=(signed char)(cross>0.0001f?1:cross< -0.0001f?-1:0);
 }
}
int cnk_session_recover_player(cnk_session *s){
 if(!s||!s->active||!s->route||s->racer_count<1)return -1;
 cnk_session_racer *r=&s->racers[0];
 if(r->race.finished||!r->race.running||r->recovery_count==UINT32_MAX)return -2;
 cnk_kart recovered=r->recovery_kart;
 if(!isfinite(recovered.position.x)||!isfinite(recovered.position.y)||
    !isfinite(recovered.position.z)||!isfinite(recovered.heading))return -3;
 recovered.speed=0;recovered.steering=0;
 recovered.ticks=r->kart.ticks;
 cnk_nav_projection proj={0};
 if(cnk_nav_nearest(s->route,recovered.position,1,&proj))return -4;
 cnk_camera next_camera=s->camera;
 if(cnk_camera_follow(&next_camera,&recovered,9,5,6,1.2f,
                      960.0f/544.0f))return -5;
 r->kart=recovered;
 r->route_fraction=proj.fraction;
 /* A teleport is never passed to cnk_race_advance, so it cannot create a
  * synthetic gate crossing, additional lap or shorter recorded lap time. */
 prime_recovered_gate_sides(s,r,recovered.position);
 r->boost=(cnk_boost_state){0};
 r->last_boost_event=0;
 r->course_direction=(cnk_route_direction){0};
 s->player_drift_held=0;
 s->player_item_requested=0;
 r->recovery_count++;
 s->camera=next_camera;
 sort_standings(s);
 return 0;
}
int cnk_session_position(const cnk_session *s,uint32_t index){
 if(!s||!s->active||index>=s->racer_count)return -1;
 for(uint32_t i=0;i<s->racer_count;i++)if(s->standings[i]==index)return (int)i+1;
 return -1;
}

int cnk_session_enable_provisional_walls(cnk_session *s,int enabled){
 if(!s||!s->active)return -1;
 s->provisional_walls_enabled=!!enabled;
 return 0;
}

int cnk_session_enable_provisional_wall_slide(cnk_session *s,int enabled){
 if(!s||!s->active)return -1;
 s->world_tuning.slide_on_contact=!!enabled;
 return 0;
}

int cnk_session_enable_provisional_kart_contacts(cnk_session *s,int enabled){
 if(!s||!s->active||cnk_kart_contact_validate(&s->contact_tuning))return -1;
 s->provisional_kart_contacts_enabled=!!enabled;
 return 0;
}

int cnk_session_set_player_drift(cnk_session *s,int held){
 if(!s||!s->active)return -1;
 s->player_drift_held=!!held;
 return 0;
}

int cnk_session_request_item(cnk_session *s){
 if(!s||!s->active)return -1;
 s->player_item_requested=1;
 return 0;
}
int cnk_session_place_item(cnk_session *s,cnk_pickup_kind kind,
                           cnk_vec3 pos,float radius,cnk_weapon reward){
 if(!s||!s->active)return -1;
 return cnk_items_place(&s->items,kind,pos,radius,reward);
}

int cnk_session_place_pad(cnk_session *s,cnk_vec3 position,float radius){
 if(!s||!s->active)return -1;
 return cnk_boost_pad_place(&s->pads,position,radius);
}
