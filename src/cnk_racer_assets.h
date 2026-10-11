#ifndef CNK_RACER_ASSETS_H
#define CNK_RACER_ASSETS_H
#include "cnk_racer_visual.h"
#include "cnk_model_lod.h"
#include "cnk_session.h"
#include "fufs.h"
#include <stdint.h>

/* Explicit source-checked 3DOB bindings, never assumed character identity.
 * TSV: legacy 19 fields = racer_slot,index,id_hex,16 affine-matrix values.
 * Optional 25 fields = racer_slot,hi_index,hi_id,mid_index,mid_id,
 * low_index,low_id,near_distance,far_distance,16 affine-matrix values.
 * LOD thresholds are MODDER-SUPPLIED, not known original Zeebo distances.
 * All source IDs must match the user's own ROM. Missing file -> no models. */
typedef struct {
  cnk_object_mesh meshes[CNK_MAX_RACERS];
  cnk_model_lod lods[CNK_MAX_RACERS];
  float calibration[CNK_MAX_RACERS][16];
  float switch_near[CNK_MAX_RACERS], switch_far[CNK_MAX_RACERS];
  uint8_t assigned[CNK_MAX_RACERS], has_lod[CNK_MAX_RACERS];
  uint32_t index[CNK_MAX_RACERS],resource_id[CNK_MAX_RACERS];
} cnk_racer_assets;

int cnk_racer_assets_load(const char *path,const fufs_file *vfs,
                          cnk_racer_assets *out);
void cnk_racer_assets_free(cnk_racer_assets *out);
/* Backward-compatible fixed attachment: highest-detail mesh for LOD slots. */
void cnk_racer_assets_attachments(const cnk_racer_assets *assets,
            cnk_racer_visual_attachment out[CNK_MAX_RACERS]);
/* Per-frame camera-based LOD selection, returns 0 and commits all
 * attachments atomically. Invalid camera/pose/thresholds: return <0 without
 * modifying output. The game remains untouched; this is presentation only. */
int cnk_racer_assets_attachments_for_camera(const cnk_racer_assets *assets,
       const cnk_session *session,cnk_racer_visual_attachment out[CNK_MAX_RACERS]);
#endif
