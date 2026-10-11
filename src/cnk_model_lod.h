#ifndef CNK_MODEL_LOD_H
#define CNK_MODEL_LOD_H
#include "cnk_object_render.h"
#include "cnk_model_texl.h"
#include <stdint.h>
/* Optional 3DOB variants of one object: detailed, intermediate, simplified.
 * All 3 (VFS index, resource ID) pairs MUST be independently verified.
 * Reconstructing likely source LOD families does not identify their racer.
 * The original game’s switch distances have NOT been recovered. */
typedef struct {
  uint32_t vfs_index;
  uint32_t source_id;
} cnk_lod_source;
typedef struct {
  cnk_object_mesh levels[3];
  cnk_lod_source source[3];
 /* A texture filename present in TEXL for all three source models. This is
  * a verified source string, NOT a resolved FUFS PNG ID or character name. */
 char shared_texture_name[CNK_TEXL_NAME_CAP];
} cnk_model_lod;
/* Atomic load and geometry sanity checks: output clear on any failure. */
int cnk_model_lod_load(const fufs_file *vfs, const cnk_lod_source source[3],
                       cnk_model_lod *out);
void cnk_model_lod_free(cnk_model_lod *set);
/* Thresholds are caller-provided pending recovery of original LOD rules.
 * Rejects non-finite/negative distances or invalid thresholds. */
const cnk_object_mesh *cnk_model_lod_choose(const cnk_model_lod *set,
                      float distance, float near_switch, float far_switch);
#endif
