#ifndef CNK_GAME_H
#define CNK_GAME_H
#include "fufs.h"
#include "cnk_scene.h"
#include "cnk_geometry.h"
#include "cnk_collision.h"
#include "cnk_render.h"
#include "cnk_session.h"
#include "cnk_track_catalog.h"
#include "cnk_minimap.h"
#include <stdint.h>

/* Native game's portable coordination layer. No BREW interpreter.
 * The VFS is borrowed: caller retains it until cnk_game_unload().
 * This is an integration milestone; the original game mechanics, HUD,
 * assets, race modes and console validation are still incomplete. */
typedef struct {
    const fufs_file *vfs;
    cnk_scene scene;
    cnk_geometry geometry;
    cnk_collision_grid collision;
    cnk_render_mesh render;
    cnk_session session;
    cnk_minimap_frame route_minimap; /* cached once per source scene */
    uint32_t scene_index;
    uint64_t simulation_frames;
    uint32_t fractional_units; /* microseconds * 60 modulo 1,000,000 */
    uint32_t last_race_events[CNK_MAX_RACERS];
    uint32_t last_boost_events;
    uint32_t last_item_events[CNK_MAX_RACERS];
    int paused;
    int loaded;
    int source_spawn_enabled; /* player kart uses original Zeebo STRT marker */
} cnk_game;

typedef struct {
    uint32_t lap, total_laps, rank, competitors;
    uint64_t elapsed_ms, last_lap_ms, best_lap_ms;
    int finished;
    int wrong_way; /* advisory only, not Zeebo HUD art */
    uint32_t recoveries; /* provisional rescue count */
    uint64_t last_sector_ms, best_sector_ms;
    uint32_t last_sector_gate; /* UINT32_MAX means no validated split */
    float kart_speed; /* signed source-world units/sec, NOT km/h */
    int reversing;
} cnk_game_hud;

/* Classification for the native results screen. Racer identities are slot
 * indices until the original Zeebo grids/character assignments are proven.
 * An unfinished kart is still RACING, never prematurely labelled DNF. */
typedef struct {
    uint32_t racer_slot;       /* original session index: 0 = local player */
    uint32_t rank;             /* 1-based finish/progress ordering */
    uint32_t completed_laps, total_laps, next_checkpoint;
    uint64_t elapsed_ms, last_lap_ms, best_lap_ms;
    int finished;
} cnk_game_classification_row;
typedef struct {
    uint32_t count;
    cnk_game_classification_row rows[CNK_MAX_RACERS];
} cnk_game_classification;
/* Pure snapshot; never changes session standings or race clocks. */
int cnk_game_classification_read(const cnk_game *g,cnk_game_classification *out);

/* Builds geometry, collision, mesh, checkpoint course, karts and camera
 * from a SCEN in the original user-supplied data.vfs. No fabricated tracks.
 * Returns 0 on success, negative on error; leaves object clear on failure.
 * Call on a zero-initialized game, or unload before reloading. */
int cnk_game_load(cnk_game *g,const fufs_file *vfs,uint32_t scene_index,
                  uint32_t racers,uint32_t laps);
/* FIDELITY PATH: the human kart starts at the original Zeebo STRT marker.
 * Opponent placement and driving model remain provisional and are NOT
 * proof of correct race gameplay. No VPK is created by this function. */
int cnk_game_load_original_start(cnk_game *g,const fufs_file *vfs,
                                 uint32_t scene_index,uint32_t racers,uint32_t laps);
/* Load a race by its ordinal within the original FUFS SCEN catalog.
 * Uses original STRT player spawn, never a fabricated track position.
 * Does not claim the original menus, modes or opponent grid are recovered. */
int cnk_game_load_catalog_race(cnk_game *g,const cnk_track_catalog *catalog,
                               uint32_t race_number,uint32_t racers,uint32_t laps);
/* Frees scene-bound memory (NOT the borrowed original VFS). */
void cnk_game_unload(cnk_game *g);
/* Advance by wall-clock microseconds in deterministic 60 Hz increments.
 * Return number of physics frames, or negative error. Maximum frame gap
 * is 250ms to prevent unbounded lag catch-up. dt_us=0 is legal.
 * The state/input model is provisional, not the original Zeebo implementation. */
int cnk_game_update(cnk_game *g,cnk_driver_input input,uint32_t dt_us);
/* A paused game's race clock and stepping stay stopped. */
int cnk_game_pause(cnk_game *g,int pause);
/* Explicit experimental opt-in. Off by default to avoid presenting
 * unverified triangle collision flags as original game behavior. */
int cnk_game_enable_provisional_walls(cnk_game *g,int enabled);
/* Test-only alternate wall response, disabled by default. */
int cnk_game_enable_provisional_wall_slide(cnk_game *g,int enabled);
/* Experimental participant-to-participant collision; off by default.
 * No claim of recovered original vehicle width or collision impulses. */
int cnk_game_enable_provisional_kart_contacts(cnk_game *g,int enabled);
/* Native player drift input (currently R on Vita). Control is deliberately
 * provisional until original Zeebo behavior is measured. */
int cnk_game_set_player_drift(cnk_game *g,int held);
/* Frontend-only one-shot during GO, before the first simulated frame. */
int cnk_game_apply_launch_boost(cnk_game *g);
int cnk_game_recover_player(cnk_game *g);
int cnk_game_take_boost_events(cnk_game *g,uint32_t *events);
int cnk_game_request_item(cnk_game *g);
int cnk_game_place_pad(cnk_game *g,cnk_vec3 position,float radius);
int cnk_game_place_item(cnk_game *g,cnk_pickup_kind kind,
                        cnk_vec3 pos,float radius,cnk_weapon reward);
int cnk_game_take_item_events(cnk_game *g,uint32_t racer,uint32_t *events);
int cnk_game_hud_read(const cnk_game *g,cnk_game_hud *out);
/* Events (CNK_RACE_GATE/LAP/FINISH) since the previous read. Clearing
 * them enables audio/UI event routing without guessing which sample plays. */
int cnk_game_take_events(cnk_game *g,uint32_t racer_index,uint32_t *events);

#endif
