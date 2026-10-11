#ifndef CNK_OBJECT_SKIN_H
#define CNK_OBJECT_SKIN_H
#include "cnk_object_render.h"
#include "cnk_vitagl_textures.h"
#include "cnk_model_texl.h"
#include <stddef.h>
#include <stdint.h>
/* Explicit, human-reviewed source associations, NOT auto-discovered names.
 * Fields are checked against the original FUFS, 3DOB TEXL and PNG bytes;
 * passing structural checks does not establish that a *new* association
 * historically belongs to Zeebo. Such claims require external evidence. */
typedef struct {
 const char *original_material_name;
 uint32_t png_vfs_index, png_vfs_id;
} cnk_object_skin_reference;
#define CNK_OBJECT_SKIN_MAX_TEXTURES 32u
typedef struct {
 cnk_object_mesh mesh;
 cnk_vitagl_textures textures;
 uint32_t selectors[CNK_OBJECT_SKIN_MAX_TEXTURES];
 unsigned count;
 int ready;
} cnk_object_skin;
int cnk_object_skin_load(const fufs_file *vfs,const cnk_png_catalog *catalog,
                         uint32_t model_index,uint32_t model_vfs_id,
                         const cnk_object_skin_reference *refs,unsigned count,
                         cnk_object_skin *out);
unsigned int cnk_object_skin_resolve(void *ctx,uint32_t selector);
void cnk_object_skin_free(cnk_object_skin *s);
#endif
