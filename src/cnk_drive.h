#ifndef CNK_DRIVE_H
#define CNK_DRIVE_H
#include "cnk_collision.h"
#include "cnk_nav.h"
#include "cnk_race.h"
#include <stdint.h>

/* Native PS Vita-port foundation: numerically safe portable kart kinematics.
 * The original Zeebo/Polarbit engine constants are not recovered; all
 * handling parameters are provisional and are exposed for later comparison.
 * NOT a claim to original tyre dynamics, drifting, boosts, jumps, AI or combat.
 */
typedef struct {
 float acceleration;      /* route world units/s^2 */
 float braking;           /* route world units/s^2 */
 float drag;              /* speed loss, units/s^2 */
 float max_speed;         /* route world units/s */
 float reverse_speed;     /* maximum reverse units/s; brake held at rest */
 float turn_rate;         /* max radians/s at normal speed */
 float wheel_response;    /* steering input smoothing per second */
 float lookahead;         /* AI aim distance in source route units */
 float min_ai_speed;      /* AI slows for large bends */
} cnk_drive_tuning;

typedef struct {
 cnk_vec3 position;
 float heading;    /* radians, 0 -> +Z, positive -> +X */
 float speed;      /* signed forward speed */
 float steering;   /* actual, filtered [-1,1] */
 uint64_t ticks;
} cnk_kart;

typedef struct { float throttle, brake, steer; } cnk_driver_input;
/* EXPERIMENTAL wall sweep, deliberately opt-in until Zeebo collision flags,
 * polygon classifications, tyre offsets and physical scales are recovered.
 * Each step sweeps a point at +probe_height against steep source faces.
 * Does NOT model real kart width or original collision impulses.
 * Experimental slide mode uses a projected displacement against two swept
 * segments to avoid tunneling through a second wall. */
typedef struct {
 float probe_height;
 float clearance;
 float max_abs_normal_y;
 /* Opt-in experimental slide response. Default 0 preserves v20 stop policy.
  * Not original Zeebo driving physics. */
 int slide_on_contact;
} cnk_world_collision_tuning;
void cnk_world_collision_default(cnk_world_collision_tuning *out);
int cnk_world_collision_validate(const cnk_world_collision_tuning *config);
/* Moves a kart using the existing integrator and prevents a mid-height
 * segment sweep from crossing steep geometry. Return 1 on wall contact,
 * 0 otherwise, <0 for an invalid or failed operation. */
int cnk_drive_step_world(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world);
int cnk_drive_race_step_world(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world,cnk_race *race,uint64_t now_ms);
/* Same world step with an optional contact report. Contacts are deliberately
 * separate from CNK_RACE_* flags, so collisions cannot masquerade as laps. */
int cnk_drive_race_step_world_report(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world,cnk_race *race,uint64_t now_ms,
 int *contact);

typedef struct {
 const cnk_sline *route; /* borrowed original DRV, closed unless configured */
 int closed;
 float lookahead;
 float target_speed;
 float route_length; /* cached immutable DRV0 arc length */
 float route_fraction;
 float route_distance;
 float progress_units;  /* unwrapped source-polyline distance */
 int progress_initialized;
} cnk_ai_driver;

void cnk_drive_default_tuning(cnk_drive_tuning *out);
int cnk_drive_validate(const cnk_drive_tuning *t);
int cnk_kart_spawn(cnk_kart *kart, cnk_vec3 position, float heading);
/* Substeps up to 1/120 s, forbids large frame times >0.10s, never clips
 * dt silently. Geometry grid may be NULL (flat/airborne mode). */
int cnk_drive_step(cnk_kart *kart,const cnk_drive_tuning *t,
                   cnk_driver_input input,float dt,
                   const cnk_collision_grid *surface);
int cnk_ai_init(cnk_ai_driver *ai,const cnk_sline *route,int closed,
                const cnk_drive_tuning *t);
/* Pure-pursuit guidance based on original DRV coordinates; not original
 * opponent AI. The same controls can drive a human and an NPC. */
int cnk_ai_controls(cnk_ai_driver *ai,const cnk_kart *kart,
                    const cnk_drive_tuning *t,cnk_driver_input *out);
/* Advances a kart and feeds movement into source-derived checkpoint tracker.
 * Returns bit flags CNK_RACE_* or a negative error code.
 */
int cnk_drive_race_step(cnk_kart *kart,const cnk_drive_tuning *t,
                        cnk_driver_input input,float dt,
                        const cnk_collision_grid *surface,
                        cnk_race *race,uint64_t now_ms);
#endif
