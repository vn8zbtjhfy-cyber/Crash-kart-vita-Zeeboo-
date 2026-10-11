#ifndef CNK_VITAGL_TEXTURES_H
#define CNK_VITAGL_TEXTURES_H
#include "cnk_png.h"
/* Optional PS Vita GPU loader for a mapping validated against ORIGINAL PNGs.
 * IMPORTANT: no texture name->VFS index association is assumed. The port must
 * supply verified cnk_texture_binding entries for each scene. */
typedef struct {
 const fufs_file *vfs;
 const cnk_png_catalog *catalog;
 unsigned int *handles;
 uint32_t count;
} cnk_vitagl_textures;
int cnk_vitagl_textures_init(cnk_vitagl_textures *set,const fufs_file *vfs,
                             const cnk_png_catalog *cat,uint32_t texture_count);
int cnk_vitagl_textures_upload(cnk_vitagl_textures *set,cnk_texture_binding binding);
unsigned int cnk_vitagl_textures_resolve(void *ctx,uint16_t index);
void cnk_vitagl_textures_free(cnk_vitagl_textures *set);
#endif
