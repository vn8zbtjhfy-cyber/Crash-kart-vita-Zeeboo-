#include "cnk_model_material.h"
#include <stdlib.h>
#include <string.h>
static int is_png(const char *s){
 size_t n=strlen(s);
 return n>4 && strcmp(s+n-4,".png")==0;
}
int cnk_model_material_inspect(const cnk_model *m,const cnk_model_texl *texl,
                               cnk_material_reference *dest,size_t cap,
                               unsigned *unresolved){
 if(!m||!texl||!dest||!unresolved||!m->groups||cap<m->num_groups)return -1;
 if(m->num_groups>512 || texl->count>CNK_TEXL_MAX_NAMES)return -2;
 cnk_material_reference stage[512]={{0}};
 unsigned missing=0;
 for(uint32_t j=0;j<m->num_groups;j++){
  uint32_t selector=m->groups[j].material;
  stage[j].original_selector=selector;
  if(selector==0)continue;
  if(selector>texl->count || !is_png(texl->names[selector-1])){
   missing++; continue;
  }
  size_t n=strlen(texl->names[selector-1]);
  memcpy(stage[j].source_png,texl->names[selector-1],n+1);
 }
 memcpy(dest,stage,(size_t)m->num_groups*sizeof(*dest));
 *unresolved=missing;
 return 0;
}
int cnk_model_material_resolve(const cnk_model *m,const cnk_model_texl *texl,
                               cnk_material_reference *dest,size_t cap){
 if(!m||!texl||!dest||!m->groups||cap<m->num_groups)return -1;
 if(m->num_groups>512 || texl->count>CNK_TEXL_MAX_NAMES)return -2;
 /* Validate everything before writing anything to the caller. */
 for(uint32_t j=0;j<m->num_groups;j++){
  uint32_t selector=m->groups[j].material;
  if(selector && (selector>texl->count ||
    !is_png(texl->names[selector-1])))return -3;
 }
 for(uint32_t j=0;j<m->num_groups;j++){
  uint32_t selector=m->groups[j].material;
  dest[j].original_selector=selector;
  dest[j].source_png[0]=0;
  if(selector){
   size_t n=strlen(texl->names[selector-1]);
   memcpy(dest[j].source_png,texl->names[selector-1],n+1);
  }
 }
 return 0;
}
int cnk_model_material_from_vfs(const fufs_file *vfs,uint32_t index,
                                 cnk_material_reference *dest,size_t dest_count,
                                 unsigned *group_count){
 unsigned char *data=NULL;size_t len=0;cnk_model m={0};cnk_model_texl t={0};
 int result=-1;
 if(group_count)*group_count=0;
 if(!vfs||!dest||!vfs->entries||index>=vfs->count)return -1;
 if(fufs_extract(vfs,index,&data,&len))return -2;
 if(cnk_parse_3dob(data,len,&m)){result=-3;goto cleanup;}
 if(cnk_model_texl_parse(data,len,&t)){result=-4;goto cleanup;}
 result=cnk_model_material_resolve(&m,&t,dest,dest_count);
 if(!result&&group_count)*group_count=m.num_groups;
cleanup:
 cnk_model_free(&m);free(data);return result;
}
