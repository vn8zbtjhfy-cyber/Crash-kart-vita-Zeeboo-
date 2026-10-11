#ifndef CNK_ITEM_SPRITE_MAP_H
#define CNK_ITEM_SPRITE_MAP_H
#include "cnk_item_visual.h"
#include "cnk_png.h"
/* Reviewed sidecar, format: SPRITE_NAME TAB VFS_INDEX TAB FUFS_ID_HEX.
 * Requiring both index and original FUFS ID prevents many mapping errors.
 * No assignments are shipped until each PNG identity is verified. */
typedef struct {
 int present[CNK_ITEM_SPRITE_COUNT];
 cnk_texture_binding binding[CNK_ITEM_SPRITE_COUNT];
 uint32_t vfs_id[CNK_ITEM_SPRITE_COUNT];
 unsigned count;
} cnk_item_sprite_map;
int cnk_item_sprite_map_read(const char *path,const fufs_file *vfs,
                             const cnk_png_catalog *catalog,cnk_item_sprite_map *out);
#endif
