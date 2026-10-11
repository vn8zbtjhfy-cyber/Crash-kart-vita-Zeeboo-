#include "cnk_verified_characters.h"
#include <string.h>

/* PNG names/IDs from Zeebo-specific publicly released asset bundles;
 * source TEXL material names and model ID checked against user's own ROM.
 * The visual association is source-assisted, not mathematical proof of a
 * general filename-to-ID mapping. NEVER infer animation/kart/driver slot. */
static const cnk_character_evidence evidence[CNK_CHARACTER_COUNT]={
 {"Crash Bandicoot","Crash.png",19u,0x0845c03fu,365u,0xee898141u},
 {"Coco Bandicoot","coco.png",308u,0xc97086b3u,305u,0xc7e85e37u},
 {"Ripper Roo","kangaroo2.png",329u,0xdbc7567fu,148u,0x5bf3392cu}
};
const cnk_character_evidence *cnk_character_original_evidence(cnk_character_identity identity){
 if((int)identity<0 || (unsigned)identity>=CNK_CHARACTER_COUNT)return NULL;
 return &evidence[identity];
}
void cnk_character_original_free(cnk_character_asset *asset){
 if(!asset)return;
 cnk_object_skin_free(&asset->skin);
 memset(asset,0,sizeof(*asset));
}
unsigned int cnk_character_original_resolve(void *ctx,uint32_t selector){
 const cnk_character_asset *asset=(const cnk_character_asset*)ctx;
 return cnk_object_skin_resolve(asset?(void*)&asset->skin:NULL,selector);
}
int cnk_character_original_load(const fufs_file *vfs,const cnk_png_catalog *catalog,
                                cnk_character_identity identity,cnk_character_asset *out){
 if(!out || !vfs || !catalog)return -1;
 memset(out,0,sizeof(*out));
 const cnk_character_evidence *ev=cnk_character_original_evidence(identity);
 if(!ev)return -2;
 const cnk_png_resource *png=cnk_png_find_index(catalog,ev->png_index);
 if(!png || png->vfs_id!=ev->png_id || png->image.width!=256 ||
    png->image.height!=256)return -3;
 cnk_object_skin_reference source={ev->original_material_name,ev->png_index,ev->png_id};
 return cnk_object_skin_load(vfs,catalog,ev->model_index,ev->model_id,
                             &source,1,&out->skin);
}

void cnk_character_pack_free(cnk_character_pack *pack){
 if(!pack)return;
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++)
  cnk_character_original_free(&pack->characters[i]);
 memset(pack,0,sizeof(*pack));
}
int cnk_character_pack_load(const fufs_file *vfs,const cnk_png_catalog *cat,
                            cnk_character_pack *out){
 if(!out || !vfs || !cat)return -1;
 memset(out,0,sizeof(*out));
 /* Precheck all original resource IDs BEFORE the first GPU upload. */
 if(!vfs->entries)return -2;
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++){
  const cnk_character_evidence *ev=&evidence[i];
  if(ev->model_index>=vfs->count||ev->png_index>=vfs->count||
     vfs->entries[ev->model_index].id!=ev->model_id ||
     vfs->entries[ev->png_index].id!=ev->png_id)return -2;
 }
 cnk_character_pack stage={0};
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++){
  if(cnk_character_original_load(vfs,cat,(cnk_character_identity)i,
                                 &stage.characters[i])){
   cnk_character_pack_free(&stage);
   return -3;
  }
 }
 stage.ready=1;
 *out=stage;
 return 0;
}
