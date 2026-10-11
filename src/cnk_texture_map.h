#ifndef CNK_TEXTURE_MAP_H
#define CNK_TEXTURE_MAP_H
#include "cnk_scene.h"
#include "cnk_png.h"
#include "fufs.h"
#include <stddef.h>
#include <stdint.h>
/* Exact identifiers from the user-supplied Zeebo ROM. This structure NEVER
 * infers a filename-to-image relationship: callers must provide evidence
 * outside the runtime. These checks prevent accidental mismatches and future
 * ROM revisions from silently assigning an unrelated texture. */
typedef struct {
 uint32_t scene_vfs_index, scene_resource_id;
 uint16_t texture_slot;
 const char *original_texture_name;
 uint32_t png_vfs_index, png_resource_id;
} cnk_texture_reference;
/* Validate all references atomically, writing slot-index bindings only when
 * every row matches this exact scene and this exact original VFS table.
 * Returns 0 for success (including an empty, unassigned map); negative error
 * for wrong scene, name, file, duplicates, invalid PNG or output size.
 * A passing check validates CONSISTENCY, not historical authenticity of the
 * supplied name->PNG association. Use a source-backed mapping only. */
int cnk_texture_map_validate(const cnk_scene *scene,
    uint32_t scene_vfs_index,const fufs_file *vfs,
    const cnk_png_catalog *catalog,const cnk_texture_reference *references,
    size_t reference_count,cnk_texture_binding *bindings,size_t capacity);


/* Check that candidate assignments agree ACROSS SCENEs before they are
 * allowed into a future verified mapping database. A material name occurring
 * in different scenes must not silently point to two distinct VFS images.
 * Resource IDs must also identify one and the same VFS entry.
 * WARNING: internal consistency does not prove historical authenticity.
 * Always call cnk_texture_map_validate per scene separately first.
 * Returns 0 on success; negative on conflict / invalid inputs.
 */
int cnk_texture_map_consistency(const cnk_texture_reference *references,
                                size_t reference_count);

#endif
