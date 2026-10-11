#include "cnk_texture_map.h"
#include <stdlib.h>
#include <string.h>

int cnk_texture_map_validate(const cnk_scene *s,
 uint32_t scene_index,const fufs_file *vfs,const cnk_png_catalog *cat,
 const cnk_texture_reference *refs,size_t n,
 cnk_texture_binding *output,size_t capacity){
 if(!s||!vfs||!vfs->entries||!cat||scene_index>=vfs->count||
    (!refs&&n)||(!output&&n)||capacity<n||n>s->texture_count||
    s->texture_count>1024||!s->texture_names)return -1;
 if(!n)return 0;
 /* Validate FIRST, write LAST: output remains unchanged on failure. */
 for(size_t i=0;i<n;i++){
  const cnk_texture_reference *r=refs+i;
  if(r->scene_vfs_index!=scene_index ||
     r->scene_resource_id!=vfs->entries[scene_index].id)return -2;
  if(r->texture_slot>=s->texture_count||!r->original_texture_name||
     !s->texture_names[r->texture_slot][0]||
     strcmp(r->original_texture_name,s->texture_names[r->texture_slot]))return -3;
  const cnk_png_resource *p=cnk_png_find_index(cat,r->png_vfs_index);
  if(r->png_vfs_index>=vfs->count||!p||
     p->vfs_id!=r->png_resource_id||
     vfs->entries[r->png_vfs_index].id!=r->png_resource_id)return -4;
  for(size_t j=0;j<i;j++)if(refs[j].texture_slot==r->texture_slot)return -5;
 }
 /* Already validated all indices. No allocations, so no partial failure. */
 for(size_t i=0;i<n;i++){
  output[i].texture_index=refs[i].texture_slot;
  output[i].vfs_index=refs[i].png_vfs_index;
 }
 return 0;
}

int cnk_texture_map_consistency(const cnk_texture_reference *r,size_t n){
 if((!r&&n)||n>8192u)return -1;
 for(size_t i=0;i<n;i++){
  const cnk_texture_reference *a=&r[i];
  if(!a->original_texture_name||!a->original_texture_name[0]||
     strlen(a->original_texture_name)>255u)return -1;
  for(size_t j=0;j<i;j++){
   const cnk_texture_reference *b=&r[j];
   /* The same named image must not acquire a different resource ID,
      even if this happens in a different scene. */
   if(!strcmp(a->original_texture_name,b->original_texture_name) &&
      (a->png_resource_id!=b->png_resource_id ||
       a->png_vfs_index!=b->png_vfs_index))return -2;
   /* FUFS IDs are authoritative identity markers for this archive. */
   if(a->png_resource_id==b->png_resource_id &&
      a->png_vfs_index!=b->png_vfs_index)return -3;
   if(a->scene_vfs_index==b->scene_vfs_index &&
      a->texture_slot==b->texture_slot)return -4;
  }
 }
 return 0;
}
