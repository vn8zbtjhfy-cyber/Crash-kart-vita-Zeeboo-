#ifndef CNK_SESSION_H
#define CNK_SESSION_H
#include "cnk_drive.h"
#include "cnk_boost.h"
#include "cnk_ai_watchdog.h"
#include "cnk_ai_tactics.h"
#include "cnk_route_direction.h"
#include "cnk_items.h"
#include "cnk_boost_pads.h"
#include "cnk_camera.h"
#include "cnk_kart_contact.h"
#define CNK_MAX_RACERS 6
/* One native race session: source-derived tracks, player, rival controllers,
 * clock, checkpoint tracking, ranking and follow camera. The coefficients,
 * spawn spacing and opponent strategy are PROVISIONAL, not reverse-engineered. */
typedef struct {
 cnk_kart kart;
 cnk_race race;
 cnk_ai_driver ai;
 cnk_route_direction course_direction; /* player-only, informational */
 uint64_t finish_ms;
 uint32_t last_event; /* source-derived lap/gate/finish events */
 cnk_boost_state boost; /* player-only provisional drifting + speed boost */
 uint32_t last_boost_event;
 uint32_t last_item_event;
 int last_wall_contact; /* provisional contact, kept out of race flags */
 uint64_t wall_contact_count;
 int last_kart_contact; /* experimental; independent of checkpoint bits */
 uint64_t kart_contact_count;
 float route_fraction;
 /* Safe recovery pose. Initial player spawn is original STRT; subsequent
  * anchors are sampled from verified source DRV0 immediately after a LAP gate.
  * This is a provisional safety feature, not an original Zeebo reset rule. */
 cnk_kart recovery_kart;
 uint32_t recovery_gate;
 uint32_t recovery_count;
 cnk_ai_watchdog ai_watchdog; /* rival-only active-simulation anti-stall */
 uint32_t last_ai_tactic; /* zero when no valid hazard/pickup decision */
} cnk_session_racer;
typedef struct {
 const cnk_scene *source; /* borrowed; must stay alive during the session */
 const cnk_sline *route;
 cnk_race_track track; /* race.track pointers refer to THIS member */
 cnk_drive_tuning tuning;
 cnk_boost_tuning boost_tuning;
 cnk_item_world items; /* inventory + opt-in verified placements */
 cnk_boost_pad_world pads; /* empty until source-reviewed pads are placed */
 int player_item_requested;
 int player_drift_held;
 cnk_session_racer racers[CNK_MAX_RACERS];
 cnk_camera camera;
 uint32_t racer_count;
 uint32_t standings[CNK_MAX_RACERS]; /* racer indices, 0 is human */
 uint64_t now_ms;
 cnk_world_collision_tuning world_tuning;
 int provisional_walls_enabled;
 int provisional_kart_contacts_enabled;
 cnk_kart_contact_tuning contact_tuning;
 int active;
} cnk_session;
/* Use the actual source STRT position/heading for the player. AI grid remains
 * provisional: the original Zeebo grid and opponents are not recovered. */
int cnk_session_start_source(cnk_session *s,const cnk_scene *scene,
                             uint32_t total_laps,uint32_t racers,uint64_t now_ms);
/* Keep validated route-aligned start for existing CPU round-trip tests. */
int cnk_session_start(cnk_session *s,const cnk_scene *scene,
                      uint32_t total_laps,uint32_t racers,uint64_t now_ms);
/* `input` is human control (racers[0]); other racers use route controller.
 * surface may be NULL for CPU test mode. Returns number newly finished or <0. */
int cnk_session_step(cnk_session *s,cnk_driver_input input,float dt,
                     uint64_t now_ms,const cnk_collision_grid *surface);
/* OFF by default, because collidability of SCEN faces is still unknown.
 * When enabled, all racers receive the same provisional wall response. */
int cnk_session_enable_provisional_walls(cnk_session *s,int enabled);
/* Select test-only wall response; has no effect unless walls are also enabled. */
int cnk_session_enable_provisional_wall_slide(cnk_session *s,int enabled);
/* EXPERIMENTAL: moving-disc collision between participants, off by default.
 * Not recovered Zeebo physics, and not enabled by ordinary race start. */
int cnk_session_enable_provisional_kart_contacts(cnk_session *s,int enabled);
int cnk_session_position(const cnk_session *s,uint32_t racer_index);
/* Manual rescue of the human racer without advancing checkpoint/clock or
 * teleporting to an arbitrary future part of the original path. */
int cnk_session_recover_player(cnk_session *s);
/* Provisional player control. Never affects opponent AI or baseline handling. */
int cnk_session_set_player_drift(cnk_session *s,int held);
/* Item use accepted only when called by the frontend after GO. */
int cnk_session_request_item(cnk_session *s);
int cnk_session_place_pad(cnk_session *s,cnk_vec3 position,float radius);
int cnk_session_place_item(cnk_session *s,cnk_pickup_kind kind,
                           cnk_vec3 position,float radius,cnk_weapon reward);
#endif
