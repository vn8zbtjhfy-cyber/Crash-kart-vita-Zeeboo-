#ifndef CNK_ITEM_LAYOUT_H
#define CNK_ITEM_LAYOUT_H
#include "cnk_game.h"
/* Optional curator-supplied positions, always tied to a VFS resource ID.
 * No entries are shipped as original until their coordinates are verified. */
#define CNK_ITEM_LAYOUT_MAX 128
typedef struct {
 uint32_t scene_resource_id;
 cnk_item_pickup pickup;
} cnk_item_layout_entry;
typedef struct {
 cnk_item_layout_entry entries[CNK_ITEM_LAYOUT_MAX];
 uint32_t count;
} cnk_item_layout;
/* Tab-separated: resource_id,kind,x,y,z,radius,reward.
 * -2 means sidecar not found. Malformed file is rejected atomically. */
int cnk_item_layout_read(const char *path,cnk_item_layout *out);
/* Applies only matching resource IDs, preserving everything on failure.
 * Return number installed, or negative. Requires a newly loaded race. */
int cnk_item_layout_apply(const cnk_item_layout *layout,cnk_game *game);
#endif
