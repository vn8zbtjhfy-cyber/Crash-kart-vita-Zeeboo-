#include "cnk_game.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

void cnk_game_unload(cnk_game *g){
    if(!g)return;
    /* Render and grid borrow geometry, while session borrows scene. */
    cnk_render_mesh_free(&g->render);
    cnk_collision_grid_free(&g->collision);
    cnk_geometry_free(&g->geometry);
    cnk_scene_free(&g->scene);
    memset(g,0,sizeof(*g));
}
static int cnk_game_load_mode(cnk_game *g,const fufs_file *vfs,
                              uint32_t scene_index,uint32_t racers,
                              uint32_t laps,int source_spawn){
    unsigned char *bytes=NULL;
    size_t len=0;
    int rc;
    if(!g||!vfs||!vfs->data||scene_index>=vfs->count||
       !racers||racers>CNK_MAX_RACERS||!laps||laps>999u)return -1;
    /* An active game must be explicitly unloaded before reinitialization. */
    if(g->loaded)return -2;
    memset(g,0,sizeof(*g));
    if(fufs_extract(vfs,scene_index,&bytes,&len)){rc=-3;goto fail;}
    if((rc=cnk_parse_scene(bytes,len,&g->scene))){rc=-4;goto fail;}
    free(bytes);bytes=NULL;
    if(cnk_geometry_build(&g->scene,1.0f/64.0f,&g->geometry)){rc=-5;goto fail;}
    if(cnk_collision_grid_build(&g->geometry,16,16,&g->collision)){rc=-6;goto fail;}
    if(cnk_render_mesh_build(&g->geometry,&g->render)){rc=-7;goto fail;}
    int start_rc=source_spawn ?
        cnk_session_start_source(&g->session,&g->scene,laps,racers,0) :
        cnk_session_start(&g->session,&g->scene,laps,racers,0);
    if(start_rc){rc=-8;goto fail;}
    /* The first native frame must also respect scene occlusion, not just
     * subsequent physics updates. This provisional camera reads only the
     * decoded original triangles, leaving unsupported geometry visible. */
    if(cnk_camera_follow_obstructed(&g->session.camera,
         &g->session.racers[0].kart,&g->collision,
         9,5,6,1.2f,960.0f/544.0f,0.4f)){rc=-9;goto fail;}
    /* Cache route silhouette once per circuit: Vita does not have to
     * resample 96 polyline positions on each rendered 60 Hz frame. */
    (void)cnk_minimap_build(&g->session,&g->route_minimap);
    g->source_spawn_enabled=!!source_spawn;
    g->vfs=vfs;
    g->scene_index=scene_index;
    g->loaded=1;
    return 0;
fail:
    free(bytes);
    cnk_game_unload(g);
    return rc;
}
int cnk_game_load(cnk_game *g,const fufs_file *vfs,uint32_t scene_index,
                  uint32_t racers,uint32_t laps){
    return cnk_game_load_mode(g,vfs,scene_index,racers,laps,0);
}
int cnk_game_load_original_start(cnk_game *g,const fufs_file *vfs,
                                 uint32_t scene_index,uint32_t racers,uint32_t laps){
    return cnk_game_load_mode(g,vfs,scene_index,racers,laps,1);
}
int cnk_game_pause(cnk_game *g,int pause){
    if(!g||!g->loaded)return -1;
    g->paused=!!pause;
    /* Discard a queued item edge on pause, never fire on resume later. */
    if(g->paused)g->session.player_item_requested=0;
    return 0;
}
int cnk_game_update(cnk_game *g,cnk_driver_input input,uint32_t dt_us){
    if(!g||!g->loaded||!g->session.active||dt_us>250000u||
       !isfinite(input.throttle)||!isfinite(input.brake)||!isfinite(input.steer))return -1;
    if(g->paused)return 0;
    uint32_t units=g->fractional_units+dt_us*60u;
    uint32_t frames=units/1000000u;
    /* Substep count is bounded: dt_us <= 250,000 means <= 15 steps. */
    for(uint32_t i=0;i<frames;i++){
        if(g->simulation_frames==UINT64_MAX)return -2;
        uint64_t next=g->simulation_frames+1;
        uint64_t now_ms=next/60u*1000u+(next%60u)*1000u/60u;
        int rc=cnk_session_step(&g->session,input,1.0f/60.0f,now_ms,
                                &g->collision);
        if(rc<0)return -3;
        g->simulation_frames=next;
        g->last_boost_events|=g->session.racers[0].last_boost_event;
        for(uint32_t k=0;k<g->session.racer_count;k++){
            g->last_race_events[k]|=g->session.racers[k].last_event;
            g->last_item_events[k]|=g->session.racers[k].last_item_event;
        }
    }
    g->fractional_units=units%1000000u;
    return (int)frames;
}
int cnk_game_hud_read(const cnk_game *g,cnk_game_hud *out){
    if(!g||!out||!g->loaded||!g->session.racer_count)return -1;
    const cnk_race *r=&g->session.racers[0].race;
    const int position=cnk_session_position(&g->session,0);
    if(position<1)return -2;
    *out=(cnk_game_hud){r->completed_laps,r->total_laps,(uint32_t)position,
                         g->session.racer_count,g->session.now_ms-r->start_ms,
                         r->last_lap_ms,r->best_lap_ms,r->finished,
                         g->session.racers[0].course_direction.wrong_way,
                         g->session.racers[0].recovery_count,
                         r->last_sector_ms,
                         r->last_sector_gate<CNK_MAX_GATES ?
                          r->best_sector_ms[r->last_sector_gate]:0,
                         r->last_sector_gate,
                         g->session.racers[0].kart.speed,
                         g->session.racers[0].kart.speed < -0.01f};
    return 0;
}

int cnk_game_classification_read(const cnk_game *g,cnk_game_classification *out){
    if(!g||!out||!g->loaded||!g->session.active ||
       !g->session.racer_count||g->session.racer_count>CNK_MAX_RACERS)return -1;
    const cnk_session *session=&g->session;
    cnk_game_classification snapshot={0};
    unsigned seen=0;
    snapshot.count=session->racer_count;
    for(uint32_t i=0;i<snapshot.count;i++){
        uint32_t slot=session->standings[i];
        if(slot>=snapshot.count || (seen&(1u<<slot)))return -2;
        seen|=1u<<slot;
        const cnk_session_racer *r=&session->racers[slot];
        if(session->now_ms<r->race.start_ms ||
           (r->race.finished&&r->finish_ms<r->race.start_ms))return -3;
        cnk_game_classification_row *row=&snapshot.rows[i];
        row->racer_slot=slot;
        row->rank=i+1u;
        row->completed_laps=r->race.completed_laps;
        row->total_laps=r->race.total_laps;
        row->next_checkpoint=r->race.next_gate;
        row->finished=!!r->race.finished;
        row->elapsed_ms=(row->finished?r->finish_ms:session->now_ms)-r->race.start_ms;
        row->last_lap_ms=r->race.last_lap_ms;
        row->best_lap_ms=r->race.best_lap_ms;
    }
    *out=snapshot;
    return 0;
}

int cnk_game_take_events(cnk_game *g,uint32_t racer_index,uint32_t *events){
    if(!g||!events||!g->loaded||racer_index>=g->session.racer_count)return -1;
    *events=g->last_race_events[racer_index];
    g->last_race_events[racer_index]=0;
    return 0;
}

int cnk_game_load_catalog_race(cnk_game *g,const cnk_track_catalog *cat,
                               uint32_t index,uint32_t racers,uint32_t laps){
 if(!g||!cat||!cat->source||!cat->source->entries)return -1;
 const cnk_track_entry *entry=cnk_track_race_at(cat,index);
 if(!entry||entry->kind!=CNK_TRACK_RACE||
    entry->vfs_index>=cat->source->count||
    cat->source->entries[entry->vfs_index].id!=entry->resource_id)return -2;
 return cnk_game_load_original_start(g,cat->source,entry->vfs_index,racers,laps);
}

int cnk_game_enable_provisional_walls(cnk_game *g,int enabled){
 if(!g||!g->loaded)return -1;
 return cnk_session_enable_provisional_walls(&g->session,enabled);
}

int cnk_game_enable_provisional_wall_slide(cnk_game *g,int enabled){
 if(!g||!g->loaded)return -1;
 return cnk_session_enable_provisional_wall_slide(&g->session,enabled);
}

int cnk_game_enable_provisional_kart_contacts(cnk_game *g,int enabled){
 if(!g||!g->loaded)return -1;
 return cnk_session_enable_provisional_kart_contacts(&g->session,enabled);
}

int cnk_game_recover_player(cnk_game *g){
 if(!g||!g->loaded||g->paused)return -1;
 int rc=cnk_session_recover_player(&g->session);
 if(!rc)g->last_boost_events=0; /* discard stale transition before reset */
 return rc;
}
int cnk_game_apply_launch_boost(cnk_game *g){
 if(!g||!g->loaded||g->paused||g->simulation_frames!=0||
    !g->session.active||!g->session.racer_count||
    g->session.racers[0].race.finished)return -1;
 cnk_boost_state *b=&g->session.racers[0].boost;
 if(b->boost_left_seconds>0.0f||b->cooldown_left_seconds>0.0f||
    b->drifting)return -2;
 /* Provisional launch pulse. The baseline kart tuning is never modified. */
 b->boost_left_seconds=0.75f;
 b->cooldown_left_seconds=0.40f;
 g->last_boost_events|=CNK_BOOST_ACTIVATED|CNK_BOOST_LAUNCH;
 return 0;
}
int cnk_game_set_player_drift(cnk_game *g,int held){
 if(!g||!g->loaded)return -1;
 return cnk_session_set_player_drift(&g->session,held);
}
int cnk_game_take_boost_events(cnk_game *g,uint32_t *events){
 if(!g||!g->loaded||!events)return -1;
 *events=g->last_boost_events;
 g->last_boost_events=0;
 return 0;
}

int cnk_game_request_item(cnk_game *g){
 if(!g||!g->loaded||g->paused)return -1;
 return cnk_session_request_item(&g->session);
}
int cnk_game_place_item(cnk_game *g,cnk_pickup_kind kind,
                        cnk_vec3 pos,float radius,cnk_weapon reward){
 if(!g||!g->loaded)return -1;
 return cnk_session_place_item(&g->session,kind,pos,radius,reward);
}
int cnk_game_take_item_events(cnk_game *g,uint32_t racer,uint32_t *events){
 if(!g||!g->loaded||!events||racer>=g->session.racer_count)return -1;
 *events=g->last_item_events[racer];
 g->last_item_events[racer]=0;
 return 0;
}

int cnk_game_place_pad(cnk_game *g,cnk_vec3 position,float radius){
 if(!g||!g->loaded)return -1;
 return cnk_session_place_pad(&g->session,position,radius);
}
