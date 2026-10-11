#ifndef CNK_GALLERY_SKINS_H
#define CNK_GALLERY_SKINS_H
#include "cnk_source_kart_gallery.h"
#include "cnk_vitagl_textures.h"
#include "cnk_model_texl.h"
/* These are RESEARCH sidecars, not automatically matched historical Zeebo
 * textures. Required TSV: 0-based family, exact TEXL filename, original
 * PNG FUFS index, original 8-hex ID. No default associations ship. */
#define CNK_GALLERY_MAX_TEXTURES 4u
#define CNK_GALLERY_MAX_SELECTORS 128u
typedef struct {uint32_t selector;unsigned texture_index;} cnk_gallery_selector;
typedef struct {
 cnk_gallery_selector refs[CNK_SOURCE_KART_LODS][CNK_GALLERY_MAX_SELECTORS];
 unsigned ref_count[CNK_SOURCE_KART_LODS],current_lod;
 cnk_vitagl_textures textures;
 int active;
} cnk_gallery_skin;
typedef struct {
 cnk_gallery_skin families[CNK_SOURCE_KART_FAMILIES];
 unsigned count;
} cnk_gallery_skins;
/* -2 means optional file is absent. All mappings preflight before ANY
 * GPU upload; on failure no live handles or partial assignments remain. */
int cnk_gallery_skins_load(const char *path,const fufs_file *vfs,
 const cnk_png_catalog *catalog,const cnk_source_kart_gallery *gallery,
 cnk_gallery_skins *out);
void cnk_gallery_skins_free(cnk_gallery_skins *skins);
int cnk_gallery_skins_bind(cnk_gallery_skins *skins,unsigned lod,
 cnk_gallery_material_binding out[CNK_SOURCE_KART_FAMILIES]);
unsigned int cnk_gallery_skin_resolve(void *ctx,uint32_t selector);
#endif
