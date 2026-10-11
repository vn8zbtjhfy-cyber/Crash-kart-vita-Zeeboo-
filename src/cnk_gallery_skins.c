#include "cnk_gallery_skins.h"
#include "cnk_model_material.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int decimal(const char *p,unsigned *v){
 unsigned out=0;if(!p||!*p)return -1;
 for(;*p;p++){
  if(*p<'0'||*p>'9'||out>(UINT32_MAX-(unsigned)(*p-'0'))/10u)return -1;
  out=out*10u+(unsigned)(*p-'0');
 }
 *v=out;return 0;
}
static int hexadecimal(const char *p,uint32_t *v){
 uint32_t out=0;if(!p||strlen(p)!=8)return -1;
 for(;*p;p++){
  unsigned n;
  if(*p>='0'&&*p<='9')n=(unsigned)(*p-'0');
  else if(*p>='a'&&*p<='f')n=(unsigned)(*p-'a'+10);
  else if(*p>='A'&&*p<='F')n=(unsigned)(*p-'A'+10);
  else return -1;
  out=(out<<4)|n;
 }
 *v=out;return 0;
}
typedef struct {char name[CNK_TEXL_NAME_CAP];unsigned idx;uint32_t id;} entry;
typedef struct {entry entries[CNK_GALLERY_MAX_TEXTURES];unsigned count;} family_spec;
void cnk_gallery_skins_free(cnk_gallery_skins *skins){
 if(!skins)return;
 for(unsigned f=0;f<CNK_SOURCE_KART_FAMILIES;f++)
  cnk_vitagl_textures_free(&skins->families[f].textures);
 memset(skins,0,sizeof(*skins));
}
unsigned int cnk_gallery_skin_resolve(void *ctx,uint32_t selector){
 cnk_gallery_skin *f=(cnk_gallery_skin*)ctx;
 if(!f||!f->active||!selector||f->current_lod>=CNK_SOURCE_KART_LODS)return 0;
 unsigned lod=f->current_lod;
 for(unsigned j=0;j<f->ref_count[lod];j++)
  if(f->refs[lod][j].selector==selector)
   return cnk_vitagl_textures_resolve(&f->textures,(uint16_t)f->refs[lod][j].texture_index);
 return 0;
}
int cnk_gallery_skins_bind(cnk_gallery_skins *skins,unsigned lod,
 cnk_gallery_material_binding out[CNK_SOURCE_KART_FAMILIES]){
 if(!skins||!out||lod>=CNK_SOURCE_KART_LODS)return -1;
 for(unsigned f=0;f<CNK_SOURCE_KART_FAMILIES;f++){
  cnk_gallery_skin *s=&skins->families[f];
  if(s->active&&(!s->textures.handles||!s->ref_count[lod]))return -2;
 }
 for(unsigned f=0;f<CNK_SOURCE_KART_FAMILIES;f++){
  cnk_gallery_skin *s=&skins->families[f];
  s->current_lod=lod;
  out[f]=(cnk_gallery_material_binding){s->active?cnk_gallery_skin_resolve:NULL,
                                      s->active?s:NULL};
 }
 return 0;
}
int cnk_gallery_skins_load(const char *path,const fufs_file *vfs,
 const cnk_png_catalog *catalog,const cnk_source_kart_gallery *gallery,
 cnk_gallery_skins *out){
 if(!path||!*path||!vfs||!vfs->data||!vfs->entries||!catalog||!gallery||
    !gallery->ready||!out)return -1;
 memset(out,0,sizeof(*out));
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 family_spec specs[CNK_SOURCE_KART_FAMILIES]={0};
 char line[512];int err=0;
 while(fgets(line,sizeof(line),fp)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1&&line[n-1]!='\n'){err=-4;break;}
  while(n&&(line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
  if(!n||line[0]=='#')continue;
  char *fields[4]={line};unsigned nf=1;
  for(char *p=line;*p;p++)if(*p=='\t'){
   if(nf==4){err=-4;break;}
   *p=0;fields[nf++]=p+1;
  }
  if(err)break;
  unsigned family,index;uint32_t id;
  if(nf!=4||decimal(fields[0],&family)||family>=CNK_SOURCE_KART_FAMILIES||
     decimal(fields[2],&index)||hexadecimal(fields[3],&id)||
     !*fields[1]||strlen(fields[1])>=CNK_TEXL_NAME_CAP||index>=vfs->count||
     vfs->entries[index].id!=id){err=-4;break;}
  const cnk_png_resource *png=cnk_png_find_index(catalog,index);
  if(!png||png->vfs_id!=id){err=-5;break;}
  family_spec *s=&specs[family];
  if(s->count>=CNK_GALLERY_MAX_TEXTURES){err=-6;break;}
  for(unsigned i=0;i<s->count;i++)if(!strcmp(s->entries[i].name,fields[1])){
   err=-6;break;
  }
  if(err)break;
  entry *e=&s->entries[s->count++];
  strcpy(e->name,fields[1]);e->idx=index;e->id=id;
 }
 if(ferror(fp))err=-3;
 fclose(fp);
 if(err)return err;
 cnk_gallery_skins stage={0};
 /* Preflight EVERY family and EVERY LOD before allocating/uploading textures.
  * All claimed materials must exist in the true 3DOB TEXL group, including
  * the primary body name at each LOD. Extras can be LOD-specific. */
 for(unsigned f=0;f<CNK_SOURCE_KART_FAMILIES;f++){
  family_spec *spec=&specs[f];
  if(!spec->count)continue;
  cnk_gallery_skin *skin=&stage.families[f];
  unsigned seen[CNK_GALLERY_MAX_TEXTURES]={0};
  for(unsigned lod=0;lod<CNK_SOURCE_KART_LODS;lod++){
   uint32_t src_index,src_id;
   cnk_material_reference mats[512]={{0}};unsigned count=0;
   if(cnk_source_kart_gallery_source(f,lod,&src_index,&src_id)||
      src_index>=vfs->count||vfs->entries[src_index].id!=src_id||
      cnk_model_material_from_vfs(vfs,src_index,mats,512,&count)||
      count!=gallery->mesh[f][lod].batch_count){err=-7;break;}
   unsigned primary_found=0;
   for(unsigned j=0;j<count;j++){
    if(gallery->mesh[f][lod].batches[j].material!=mats[j].original_selector){err=-7;break;}
    for(unsigned k=0;k<spec->count;k++)if(!strcmp(spec->entries[k].name,mats[j].source_png)){
     if(!mats[j].original_selector){err=-7;break;}
     unsigned t=0;
     for(;t<skin->ref_count[lod];t++)
      if(skin->refs[lod][t].selector==mats[j].original_selector)break;
     if(t<skin->ref_count[lod]){
      if(skin->refs[lod][t].texture_index!=k){err=-7;break;}
     }else{
      if(t>=CNK_GALLERY_MAX_SELECTORS){err=-7;break;}
      skin->refs[lod][t]=(cnk_gallery_selector){mats[j].original_selector,k};
      skin->ref_count[lod]++;
     }
     if(k==0)primary_found++;
     seen[k]++;
    }
    if(err)break;
   }
   if(err||!primary_found){err=-7;break;}
  }
  if(err)break;
  for(unsigned k=0;k<spec->count;k++)if(!seen[k]){err=-7;break;}
  if(err)break;
 }
 if(err)return err; /* no GPU resources allocated yet */
 for(unsigned f=0;f<CNK_SOURCE_KART_FAMILIES;f++){
  family_spec *spec=&specs[f];
  if(!spec->count)continue;
  cnk_gallery_skin *skin=&stage.families[f];
  if(cnk_vitagl_textures_init(&skin->textures,vfs,catalog,spec->count)){
   cnk_gallery_skins_free(&stage);return -8;
  }
  for(unsigned j=0;j<spec->count;j++)
   if(cnk_vitagl_textures_upload(&skin->textures,
      (cnk_texture_binding){(uint16_t)j,spec->entries[j].idx})){
    cnk_gallery_skins_free(&stage);return -8;
   }
  skin->active=1;stage.count++;
 }
 *out=stage;return 0;
}
