#ifndef CNK_VERIFIED_CHARACTERS_H
#define CNK_VERIFIED_CHARACTERS_H
#include "cnk_object_skin.h"
#include <stdint.h>

/* Independently published character PNG IDs which are also present in the
 * supplied Zeebo ROM. The model/material names below are independently read
 * from 3DOB/TEXL in that SAME ROM. These are source-assisted associations,
 * not a solved FUFS filename hash and NOT driver-to-kart assignments.
 * Requires an initialized GL context for texture uploads. */
typedef enum {
 CNK_CHARACTER_CRASH=0,
 CNK_CHARACTER_COCO=1,
 CNK_CHARACTER_RIPPER_ROO=2,
 CNK_CHARACTER_COUNT=3
} cnk_character_identity;

typedef struct {
 const char *name;
 const char *original_material_name;
 uint32_t model_index, model_id;
 uint32_t png_index, png_id;
} cnk_character_evidence;

typedef struct {cnk_object_skin skin;} cnk_character_asset;

const cnk_character_evidence *cnk_character_original_evidence(cnk_character_identity identity);
int cnk_character_original_load(const fufs_file *vfs,const cnk_png_catalog *catalog,
                                cnk_character_identity identity,cnk_character_asset *out);
unsigned int cnk_character_original_resolve(void *ctx,uint32_t selector);
void cnk_character_original_free(cnk_character_asset *asset);
/* All three documented character skins loaded atomically: if one source/GPU
 * operation fails, earlier meshes/textures are also released. No character
 * is automatically placed into a race or assigned to an unverified kart. */
typedef struct {
 cnk_character_asset characters[CNK_CHARACTER_COUNT];
 int ready;
} cnk_character_pack;
int cnk_character_pack_load(const fufs_file *vfs,const cnk_png_catalog *catalog,
                            cnk_character_pack *out);
void cnk_character_pack_free(cnk_character_pack *pack);

#endif
