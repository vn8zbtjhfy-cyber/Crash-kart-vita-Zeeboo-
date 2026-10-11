#ifndef CNK_VITAGL_SCENE_ASSETS_H
#define CNK_VITAGL_SCENE_ASSETS_H
#include "cnk_vitagl_textures.h"
#include "cnk_texture_map.h"
/* Upload scene textures only from explicit, externally verified source-backed
 * name->FUFS references. No guessed mappings and no placeholder images.
 * Validates all references before any GPU calls. If an upload fails, deletes
 * only the textures created by THIS call; already loaded slots remain intact.
 * Empty references are legal and make no GL calls.
 * Call after cnk_vitagl_textures_init with count >= scene->texture_count.
 * Zero = success. Negative values denote invalid input or GPU failure.
 * IMPORTANT: structural validation is not proof of the historical mapping. */
int cnk_vitagl_scene_upload_verified(cnk_vitagl_textures *set,
    const cnk_scene *scene, uint32_t scene_vfs_index,
    const cnk_texture_reference *references, size_t reference_count);
#endif
