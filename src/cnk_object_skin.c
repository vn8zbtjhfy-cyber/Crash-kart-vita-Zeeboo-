#include "cnk_object_skin.h"
#include "cnk_model_material.h"
#include <string.h>
void cnk_object_skin_free(cnk_object_skin *s){
 if(!s)return;
 cnk_vitagl_textures_free(&s->textures);
 cnk_object_mesh_free(&s->mesh);
 memset(s,0,sizeof(*s));
}
unsigned int cnk_object_skin_resolve(void *ctx,uint32_t selector){
 const cnk_object_skin *s=(const cnk_object_skin*)ctx;
 if(!s||!s->ready||!selector)return 0;
 for(unsigned i=0;i<s->count;i++)if(s->selectors[i]==selector)
   return cnk_vitagl_textures_resolve((void*)&s->textures,(uint16_t)i);
 return 0;
}
int cnk_object_skin_load(const fufs_file *vfs,const cnk_png_catalog *cat,
 uint32_t model_index,uint32_t model_id,const cnk_object_skin_reference *refs,
 unsigned count,cnk_object_skin *out){
 if(!out||!vfs||!vfs->entries||!vfs->data||!cat||!refs||
    !count||count>CNK_OBJECT_SKIN_MAX_TEXTURES ||model_index>=vfs->count)return -1;
 memset(out,0,sizeof(*out));
 if(vfs->entries[model_index].id!=model_id)return -2;
 cnk_material_reference mats[512]={{0}};
 unsigned groups=0;
 if(cnk_model_material_from_vfs(vfs,model_index,mats,512,&groups)||!groups)return -3;
 uint32_t selectors[CNK_OBJECT_SKIN_MAX_TEXTURES]={0};
 for(unsigned k=0;k<count;k++){
  const cnk_object_skin_reference *r=&refs[k];
  if(!r->original_material_name||!r->original_material_name[0]||
      strlen(r->original_material_name)>=CNK_TEXL_NAME_CAP||
      r->png_vfs_index>=vfs->count ||
      vfs->entries[r->png_vfs_index].id!=r->png_vfs_id)return -4;
  const cnk_png_resource *png=cnk_png_find_index(cat,r->png_vfs_index);
  if(!png||png->vfs_id!=r->png_vfs_id)return -5;
  unsigned hits=0;
  for(unsigned i=0;i<groups;i++)if(!strcmp(mats[i].source_png,r->original_material_name)){
   if(!mats[i].original_selector)return -6;
   if(selectors[k] && selectors[k]!=mats[i].original_selector)return -6;
   selectors[k]=mats[i].original_selector;
   hits++;
  }
  if(!hits)return -6;
  for(unsigned j=0;j<k;j++)if(selectors[j]==selectors[k] ||
        !strcmp(refs[j].original_material_name,r->original_material_name))return -7;
 }
 cnk_object_skin stage={0};
 if(cnk_object_mesh_from_vfs(vfs,model_index,&stage.mesh))return -8;
 if(stage.mesh.batch_count!=groups){cnk_object_skin_free(&stage);return -9;}
 for(unsigned i=0;i<groups;i++)if(stage.mesh.batches[i].material!=mats[i].original_selector){
  cnk_object_skin_free(&stage);return -9;
 }
 if(cnk_vitagl_textures_init(&stage.textures,vfs,cat,count)){
  cnk_object_skin_free(&stage);return -10;
 }
 for(unsigned k=0;k<count;k++){
  if(cnk_vitagl_textures_upload(&stage.textures,
      (cnk_texture_binding){(uint16_t)k,refs[k].png_vfs_index})){
   cnk_object_skin_free(&stage);return -11;
  }
  stage.selectors[k]=selectors[k];
 }
 stage.count=count;stage.ready=1;
 *out=stage;
 return 0;
}
