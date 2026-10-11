#ifndef CNK_VITAGL_GAME_FRAME_H
#define CNK_VITAGL_GAME_FRAME_H

#include "cnk_game.h"
#include "cnk_vitagl_render.h"
#include "cnk_racer_visual.h"
#include "cnk_vitagl_items.h"

/* Bridge between the real source-SCEN race session and the native Vita GPU.
 * This does not draw karts, original HUD, menus or unmapped textures yet.
 * All positions, camera matrices and track polygons come from cnk_game.
 * This entry point contains no emulator and does not produce a VPK. */
typedef struct {
    cnk_game_hud race;
    unsigned track_draw_calls;
    unsigned racer_draw_calls;
    unsigned racer_models_drawn;
    uint32_t racer_triangles;
    uint32_t visible_track_triangles;
    unsigned driver_models_drawn;
    unsigned driver_draw_calls;
    uint32_t driver_triangles;
    uint64_t simulation_frames;
    unsigned item_draw_calls;
    uint32_t item_actors;
    uint32_t item_actors_culled;
} cnk_vitagl_frame_result;

/* Returns 0 on success. Invalid/unfinished game returns <0 without touching
 * the graphics API. No texture assignments are fabricated: unresolved material
 * handles go through the existing untextured path. */
/* Optional racer attachments must be explicit, source-verified and calibrated.
 * NULL = draw tracks only. A non-NULL array has CNK_MAX_RACERS slots.
 * Preflight rejects invalid attachments BEFORE any GPU operation.
 */
int cnk_vitagl_draw_game_frame_with_racers(const cnk_game *game,
                              cnk_native_texture_resolve resolve,
                              void *context,
                              const cnk_racer_visual_attachment *attachments,
                              cnk_vitagl_frame_result *out);
int cnk_vitagl_draw_game_frame(const cnk_game *game,
                              cnk_native_texture_resolve resolve,
                              void *context,
                              cnk_vitagl_frame_result *out);
/* Opt-in to conservative CPU-side scene culling for Vita rendering. */
int cnk_vitagl_draw_game_frame_with_racers_culled(const cnk_game *game,
                              cnk_native_texture_resolve resolve,void *context,
                              const cnk_racer_visual_attachment *attachments,
                              cnk_vitagl_frame_result *out);
/* Draw original kart + separately source-verified textured driver per slot.
 * Driver placement requires explicit reviewed sidecar calibration. All
 * supplied attachments are prevalidated BEFORE the first GPU command. */
int cnk_vitagl_draw_game_frame_with_actors_culled(const cnk_game *game,
                              cnk_native_texture_resolve resolve,void *context,
                              const cnk_racer_visual_attachment *karts,
                              const cnk_racer_visual_attachment *drivers,
                              cnk_vitagl_frame_result *out);
/* Displays authoritative projectiles and traps as world-space billboards.
 * No invented spawn locations; original PNGs require explicit verified map. */
int cnk_vitagl_draw_game_frame_with_items_culled(const cnk_game *game,
           cnk_native_texture_resolve resolve,void *context,
           const cnk_racer_visual_attachment *karts,
           const cnk_racer_visual_attachment *drivers,
           cnk_item_sprite_resolve item_resolve,void *item_context,
           cnk_vitagl_frame_result *out);
#endif
