#ifndef CNK_PNG_H
#define CNK_PNG_H
#include "fufs.h"
#include <stddef.h>
#include <stdint.h>
/* PNG metadata / CRC validation. No image is relabelled with a guessed name. */
typedef struct {
 uint32_t width,height; uint8_t bit_depth,color_type,compression,filter,interlace;
} cnk_png_info;
/* 0=verified PNG; negative codes distinguish unsupported, truncated and corrupt. */
int cnk_png_inspect(const unsigned char *data,size_t size,cnk_png_info *out);
/* Collect only original PNG resources, keeping their exact FUFS table index/id. */
typedef struct {uint32_t vfs_index,vfs_id;cnk_png_info image;} cnk_png_resource;
typedef struct {cnk_png_resource *items;size_t count;} cnk_png_catalog;
int cnk_png_catalog_build(const fufs_file *f,cnk_png_catalog *out);
void cnk_png_catalog_free(cnk_png_catalog *cat);
const cnk_png_resource *cnk_png_find_index(const cnk_png_catalog *cat,uint32_t vfs_index);
/* A binding is accepted only when it refers to a known, validated PNG. */
typedef struct {uint16_t texture_index;uint32_t vfs_index;} cnk_texture_binding;
int cnk_png_binding_check(const cnk_png_catalog *cat,uint32_t scene_texture_count,
                          const cnk_texture_binding *entries,size_t n);
#endif
