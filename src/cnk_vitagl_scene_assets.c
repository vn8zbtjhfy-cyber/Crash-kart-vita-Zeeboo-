#include "cnk_vitagl_scene_assets.h"
#include <vitaGL.h>
#include <stdlib.h>

int cnk_vitagl_scene_upload_verified(cnk_vitagl_textures *set,
    const cnk_scene *scene, uint32_t scene_vfs_index,
    const cnk_texture_reference *refs, size_t count){
 if(!set||!set->vfs||!set->catalog||!set->handles||!scene||
    (count&&!refs)||count>scene->texture_count||
    scene->texture_count>set->count||scene->texture_count>1024u)return -1;
 if(!count)return 0;
 if(cnk_texture_map_consistency(refs,count))return -2;
 cnk_texture_binding *binding=(cnk_texture_binding*)calloc(count,sizeof(*binding));
 if(!binding)return -3;
 if(cnk_texture_map_validate(scene,scene_vfs_index,set->vfs,set->catalog,
                             refs,count,binding,count)){
  free(binding);return -4;
 }
 /* Fail before any GL work if one requested slot is already occupied. */
 for(size_t i=0;i<count;i++)if(set->handles[binding[i].texture_index]){
  free(binding);return -5;
 }
 for(size_t i=0;i<count;i++){
  if(cnk_vitagl_textures_upload(set,binding[i])){
   /* Roll back ONLY textures created during this invocation. Future callers
    * may retry this complete set, without changing pre-existing slots. */
   for(size_t j=0;j<i;j++){
    uint16_t slot=binding[j].texture_index;
    GLuint h=(GLuint)set->handles[slot];
    if(h){glDeleteTextures(1,&h);set->handles[slot]=0;}
   }
   free(binding);return -6;
  }
 }
 free(binding);return 0;
}
