#ifndef CNK_TEXTURE_MANIFEST_H
#define CNK_TEXTURE_MANIFEST_H
#include "cnk_texture_map.h"
/* Optional explicit associations from a separately reviewed source. No
 * original image-to-name associations are guessed, bundled or inferred.
 * File format: TSV with six fields, one association per line:
 * scene_vfs_index \t scene_resource_id(hex) \t texture_slot \t
 * original_texture_name \t png_vfs_index \t png_resource_id(hex)
 * Lines beginning with # are comments. IDs are eight hexadecimal digits.
 * Parsing and internal consistency alone do NOT prove authenticity. */
#define CNK_MANIFEST_MAX_REFS 4096u
#define CNK_MANIFEST_NAME_CAP 96u

typedef struct {
 uint32_t scene_vfs_index,scene_resource_id,png_vfs_index,png_resource_id;
 uint16_t texture_slot;
 char original_texture_name[CNK_MANIFEST_NAME_CAP];
} cnk_manifest_row;
typedef struct {
 cnk_manifest_row *rows;
 size_t count;
} cnk_texture_manifest;
/* Parse entire document or fail atomically, leaving out empty.
 * File missing = -2; malformed/duplicate/conflicting lines = other negatives. */
int cnk_texture_manifest_read(const char *path,cnk_texture_manifest *out);
void cnk_texture_manifest_free(cnk_texture_manifest *manifest);
/* Extract refs for just this scene, then verify original names, scene IDs and
 * PNG IDs in the exact user ROM. Caller provides >= scene->texture_count slots.
 * If no refs exist returns 0 and n=0 (untextured, never guessed).
 * Does not touch output on failure. */
int cnk_texture_manifest_scene(const cnk_texture_manifest *manifest,
    const cnk_scene *scene,uint32_t scene_vfs_index,const fufs_file *vfs,
    const cnk_png_catalog *catalog,cnk_texture_reference *references,
    size_t capacity,size_t *out_count);
/* Strictly checks ALL referenced scene resources against source VFS + PNG
 * catalog BEFORE any draw/upload. Missing/misidentified source fails. */
int cnk_texture_manifest_verify_all(const cnk_texture_manifest *manifest,
                                    const fufs_file *vfs,const cnk_png_catalog *catalog);
#endif
