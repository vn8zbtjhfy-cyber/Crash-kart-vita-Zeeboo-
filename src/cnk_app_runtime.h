#ifndef CNK_APP_RUNTIME_H
#define CNK_APP_RUNTIME_H
#include "cnk_frontend_actions.h"
#include "cnk_item_layout.h"
#include "cnk_pad_layout.h"
#include "cnk_minimap.h"
#include "cnk_records.h"
#include "cnk_cup_checkpoint.h"
#include "cnk_story_checkpoint.h"
#include "cnk_collision_probe.h"
#include <stdint.h>

/* Portable EVENT ROUTER for the future original Zeebo audio, HUD and visual
 * effects. Signals are generated from actual native frontend transitions,
 * not guesses about which source sound or animation should play. */
#define CNK_APP_EVENT_TRACK_CHANGED (1u << 0)
#define CNK_APP_EVENT_RACE_STARTED  (1u << 1)
#define CNK_APP_EVENT_RACE_RESTARTED (1u << 2)
#define CNK_APP_EVENT_PAUSED        (1u << 3)
#define CNK_APP_EVENT_RESUMED       (1u << 4)
#define CNK_APP_EVENT_BACK_TO_SELECT (1u << 5)
#define CNK_APP_EVENT_RESULTS       (1u << 6)
#define CNK_APP_EVENT_COUNT_3       (1u << 7)
#define CNK_APP_EVENT_COUNT_2       (1u << 8)
#define CNK_APP_EVENT_COUNT_1       (1u << 9)
#define CNK_APP_EVENT_GO            (1u << 10)
#define CNK_APP_EVENT_DRIFT_START   (1u << 11)
#define CNK_APP_EVENT_DRIFT_STOP    (1u << 12)
#define CNK_APP_EVENT_BOOST_START   (1u << 13)
#define CNK_APP_EVENT_BOOST_END     (1u << 14)
#define CNK_APP_EVENT_ITEM_COLLECTED (1u << 15)
#define CNK_APP_EVENT_WUMPA_COLLECTED (1u << 16)
#define CNK_APP_EVENT_ITEM_FIRED    (1u << 17)
#define CNK_APP_EVENT_ITEM_HIT      (1u << 18)
#define CNK_APP_EVENT_MISSILE_FIRED (1u << 19)
#define CNK_APP_EVENT_HAZARD_DROPPED (1u << 20)
#define CNK_APP_EVENT_MISSILE_HIT (1u << 21)
#define CNK_APP_EVENT_TNT_HIT (1u << 22)
#define CNK_APP_EVENT_OIL_HIT (1u << 23)
#define CNK_APP_EVENT_WRONG_WAY_START (1u << 24)
#define CNK_APP_EVENT_WRONG_WAY_CLEAR (1u << 25)
#define CNK_APP_EVENT_KART_RECOVERED (1u << 26)
#define CNK_APP_EVENT_NEW_RECORD (1u << 27)
#define CNK_APP_EVENT_MODE_CHANGED (1u << 28)
#define CNK_APP_EVENT_LAPS_CHANGED (1u << 29)
#define CNK_APP_EVENT_LAUNCH_BOOST (1u << 30)
#define CNK_APP_EVENT_INCOMING_PROJECTILE (1u << 31)

/* Platform independent application lifecycle, linking the authentic VFS
 * source to the native race frontend. The timing, menus and physics are
 * still a reconstruction rather than recovered BREW game logic. */
typedef struct {
    cnk_frontend frontend;
    cnk_frontend_action_state input_edges;
    cnk_records records; /* provisional LOCAL native race records */
    char records_path[CNK_RECORDS_PATH_LIMIT];
    char cup_checkpoint_path[CNK_CUP_CHECKPOINT_PATH_MAX];
    int cup_write_error; /* nonfatal, completed-round checkpoint only */
    char story_checkpoint_path[CNK_STORY_CHECKPOINT_PATH_MAX];
    int story_write_error; /* nonfatal, completed-stage checkpoint only */
    cnk_story completed_story_archive; /* validated final DEV story ranks */
    cnk_cup completed_cup_archive; /* last verified final standings, DEV only */
    int records_write_error; /* storage I/O cannot stop a running race */
    const cnk_item_layout *item_layout; /* borrowed optional verified positions */
    const cnk_pad_layout *pad_layout; /* borrowed optional verified strip locations */
    cnk_collision_probe collision_probe; /* explicit developer opt-in only */
    uint64_t last_clock_us;
    uint64_t rendered_frames;
    uint32_t last_delta_us;
    int last_projectile_warning; /* transition state, never persistent sound */
    int clock_initialized;
    int initialized;
} cnk_app_runtime;

typedef struct {
    cnk_frontend_state state;
    uint32_t selected_race;
    uint32_t available_races;
    uint32_t selected_vfs_index;
    uint32_t source_resource_id;
    int simulated_steps;
    uint64_t rendered_frames;
    cnk_game_hud hud;
    cnk_game_classification classification;
    cnk_cup_standings cup_standings;
    int cup_active;
    int cup_available; /* opt-in source-validated developer manifest */
    int story_available; /* opt-in developer objectives on authentic tracks */
    int story_active;
    int story_last_attempt_success;
    uint32_t story_completed, story_total;
    int has_completed_story;
    uint32_t completed_story_count;
    uint8_t completed_story_ranks[CNK_STORY_MAX_STAGES];
    int story_write_error;
    cnk_cup_standings completed_cup_standings; /* persisted final rankings */
    int has_completed_cup; /* separate from a currently running Cup */
    int cup_write_error;
    int practice_mode;
    uint32_t configured_laps;
    cnk_record personal_best; /* zero if no local record */
    int has_personal_best;
    int records_write_error;
    cnk_minimap_frame minimap; /* original DRV0 silhouette and racer positions */
    cnk_boost_state player_boost; /* live native player status, not original HUD */
    float boost_charge_fraction;
    int boost_active;
    cnk_item_racer player_items;
    /* Source-LANG-backed effects; duration/tuning still provisional. */
    float clock_seconds, candle_seconds;
    int clock_affects_player, candle_affects_player;
    uint32_t item_events[CNK_MAX_RACERS];
    int incoming_projectile; /* advisory only, current 2D collision prediction */
    cnk_weapon incoming_kind;
    uint32_t incoming_eta_ms;
    float incoming_lateral_side;
    uint32_t active_missiles; /* provisional actors, not original graphics */
    uint32_t active_hazards;
    uint32_t placed_pickups;
    uint32_t placed_pads;
    /* Ephemeral signals, reset on EVERY tick: UI transitions plus per-kart
       checkpoint, lap and finish flags from the source-backed race gates. */
    uint32_t app_events;
 cnk_start_phase start_phase;
 uint32_t start_phase_remaining_ms;
    uint32_t race_events[CNK_MAX_RACERS];
    int has_hud;
} cnk_app_snapshot;

/* A caller owns and keeps VFS open for this runtime's lifetime. */
int cnk_app_runtime_init(cnk_app_runtime *app,const fufs_file *vfs,
                         uint32_t racers,uint32_t laps);
/* Read-only snapshot, including original source scene IDs for selection. */
int cnk_app_runtime_snapshot(const cnk_app_runtime *app,cnk_app_snapshot *out);
/* Tick once per display frame. Delta is clamped to avoid a runaway
 * catch-up burst after a suspend, resume or stalled frame. Monotonic
 * timestamps are required; regression is treated as a clock reset. */
int cnk_app_runtime_tick(cnk_app_runtime *app,uint64_t now_us,
                         uint32_t held_ui_buttons,cnk_driver_input driving,
                         cnk_app_snapshot *snapshot);
/* Optional local save. A missing file is accepted; malformed files are
 * rejected without changing the in-memory records. */
int cnk_app_runtime_set_records_path(cnk_app_runtime *app,const char *path);
/* Must be called after layouts/other setup. A corrupt matching checkpoint
 * is NOT overwritten automatically, and does not prevent ordinary races. */
int cnk_app_runtime_set_cup_checkpoint_path(cnk_app_runtime *app,const char *path);
int cnk_app_runtime_set_story_checkpoint_path(cnk_app_runtime *app,const char *path);
int cnk_app_runtime_set_item_layout(cnk_app_runtime *app,const cnk_item_layout *layout);
int cnk_app_runtime_set_pad_layout(cnk_app_runtime *app,const cnk_pad_layout *layout);
/* SELECT-only, applies opt-in experimental world/kart contacts on each start.
 * Defaults remain off. Neither setting claims original collision materials. */
int cnk_app_runtime_set_collision_probe(cnk_app_runtime *app,cnk_collision_probe mode);
void cnk_app_runtime_close(cnk_app_runtime *app);
#endif
