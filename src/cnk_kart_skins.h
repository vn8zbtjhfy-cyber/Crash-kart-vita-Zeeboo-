#ifndef CNK_KART_SKINS_H
#define CNK_KART_SKINS_H
#include "cnk_racer_assets.h"
#include "cnk_png.h"
#include "cnk_vitagl_textures.h"

/* A source-checked optional mapping of kart body AND auxiliary materials.
 * Each row: slot<TAB>original TEXL filename<TAB>FUFS index<TAB>ID(8hex).
 * The mapping is supplied externally ONLY after independent review.
 * This module never claims that a PNG belongs to a given character or kart.
 * The FIRST material in each slot MUST appear in all original LOD meshes.
 * Extra materials (such as wheels) may appear in only some LOD meshes.
 */
#define CNK_KART_MAX_TEXTURES 4
#define CNK_KART_MAX_SELECTORS 512
typedef struct {uint32_t selector;unsigned texture_index;} cnk_kart_selector_binding;
typedef struct {
 cnk_vitagl_textures textures;
 cnk_kart_selector_binding materials[3][CNK_KART_MAX_SELECTORS];
 unsigned material_count[3];
 unsigned texture_count;
 const cnk_object_mesh *models[3];
 unsigned levels;
 unsigned current_level;
 int active;
} cnk_kart_skin;
typedef struct {
 cnk_kart_skin slots[CNK_MAX_RACERS];
 unsigned count;
} cnk_kart_skins;

/* -2: optional sidecar missing. On ANY other error *out is empty and any
 * previously uploaded GPU textures are deleted. Requires active VitaGL. */
int cnk_kart_skins_load(const char *path,const fufs_file *vfs,
     const cnk_png_catalog *cat,const cnk_racer_assets *racers,
     cnk_kart_skins *out);
void cnk_kart_skins_free(cnk_kart_skins *set);
/* Atomically validate current LOD/mesh selections, then bind the verified
 * texture callbacks. Unassigned slots remain unchanged. */
int cnk_kart_skins_apply(cnk_kart_skins *set,
     cnk_racer_visual_attachment attachments[CNK_MAX_RACERS]);
unsigned int cnk_kart_skins_resolve(void *ctx,uint32_t material);
#endif
