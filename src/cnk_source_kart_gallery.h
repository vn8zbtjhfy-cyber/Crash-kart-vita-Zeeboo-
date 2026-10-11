#ifndef CNK_SOURCE_KART_GALLERY_H
#define CNK_SOURCE_KART_GALLERY_H
#include "cnk_object_render.h"
#include "cnk_camera.h"
#include "cnk_source_kart_catalog.h"

/* Six independently source-identified Zeebo vehicle mesh families. Each family
 * has three source LODs. These meshes are NOT assigned to characters or races;
 * body-texture mapping and original showroom layout remain unverified. */
#define CNK_SOURCE_KART_FAMILIES 6u
#define CNK_SOURCE_KART_LODS 3u
typedef struct {
 cnk_object_mesh mesh[CNK_SOURCE_KART_FAMILIES][CNK_SOURCE_KART_LODS];
 int ready;
} cnk_source_kart_gallery;
/* Atomic on error: *out remains empty, even if a later source model is corrupt. */
int cnk_source_kart_gallery_load(const fufs_file *vfs,cnk_source_kart_gallery *out);
void cnk_source_kart_gallery_free(cnk_source_kart_gallery *gallery);
/* Materials are deliberately separate from the gallery's original meshes.
 * No bindings => the six models remain untextured. */
typedef struct {
 unsigned int (*resolve)(void *ctx,uint32_t selector);
 void *context;
} cnk_gallery_material_binding;
int cnk_source_kart_gallery_draw_mapped_at(const cnk_source_kart_gallery *gallery,
 unsigned detail, float angle,
 const cnk_gallery_material_binding bindings[CNK_SOURCE_KART_FAMILIES],
 unsigned *draw_calls);
int cnk_source_kart_gallery_draw_mapped(const cnk_source_kart_gallery *gallery,
 unsigned detail,const cnk_gallery_material_binding bindings[CNK_SOURCE_KART_FAMILIES],
 unsigned *draw_calls);
/* 0=highest-detail, 1=medium, 2=lowest-detail, from ORIGINAL ROM meshes. */
int cnk_source_kart_gallery_draw(const cnk_source_kart_gallery *gallery,
                                unsigned detail,unsigned *calls);
int cnk_source_kart_gallery_camera_at(const cnk_object_mesh *mesh,float angle,cnk_camera *out);
int cnk_source_kart_gallery_camera(const cnk_object_mesh *mesh,cnk_camera *out);
#endif
