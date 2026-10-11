#ifndef CNK_PAD_LAYOUT_H
#define CNK_PAD_LAYOUT_H
#include "cnk_game.h"
/* Authored/reviewed original scene coordinates only. An EMPTY source file
 * is deliberate: absence of verified placements never creates fake pads. */
#define CNK_PAD_LAYOUT_MAX 128
typedef struct {
 uint32_t scene_resource_id;
 cnk_vec3 position;
 float radius;
} cnk_pad_layout_entry;
typedef struct {
 cnk_pad_layout_entry entries[CNK_PAD_LAYOUT_MAX];
 uint32_t count;
} cnk_pad_layout;
/* Strict tab-separated: source_ID x y z radius. -2 = absent sidecar.
 * Parse and per-scene apply are atomic and preserve previous values on error. */
int cnk_pad_layout_read(const char *path,cnk_pad_layout *out);
int cnk_pad_layout_apply(const cnk_pad_layout *layout,cnk_game *game);
#endif
