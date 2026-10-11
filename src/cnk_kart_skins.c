#include "cnk_kart_skins.h"
#include "cnk_model_material.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int dec(const char *s,unsigned *out){
 unsigned n=0;
 if(!s||!*s)return -1;
 for(;*s;s++){
  if(*s<'0'||*s>'9'||n>(UINT32_MAX-(unsigned)(*s-'0'))/10u)return -1;
  n=n*10u+(unsigned)(*s-'0');
 }
 *out=n;return 0;
}
static int hex(const char *s,uint32_t *out){
 if(!s||strlen(s)!=8)return -1;
 uint32_t n=0;
 for(;*s;s++){
  unsigned c=(unsigned char)*s,v=0;
  if(c>='0'&&c<='9')v=c-'0';
  else if(c>='a'&&c<='f')v=c-'a'+10;
  else if(c>='A'&&c<='F')v=c-'A'+10;
  else return -1;
  n=(n<<4)|v;
 }
 *out=n;return 0;
}
void cnk_kart_skins_free(cnk_kart_skins *s){
 if(!s)return;
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)cnk_vitagl_textures_free(&s->slots[i].textures);
 memset(s,0,sizeof(*s));
}
unsigned int cnk_kart_skins_resolve(void *ctx,uint32_t material){
 const cnk_kart_skin *s=(const cnk_kart_skin *)ctx;
 if(!s||!s->active||s->current_level>=s->levels||!material)return 0;
 for(unsigned i=0;i<s->material_count[s->current_level];i++)
  if(material==s->materials[s->current_level][i].selector)
   return cnk_vitagl_textures_resolve((void *)&s->textures,
           (uint16_t)s->materials[s->current_level][i].texture_index);
 return 0;
}
/* A TEXL name may appear in multiple POLY groups with different source
 * selectors (e.g. the wheels in the simplified original kart model).
 * Keep ALL selectors, without guessing a single representative selector. */
static int collect_materials(const fufs_file *vfs,uint32_t idx,
            const char names[CNK_KART_MAX_TEXTURES][CNK_TEXL_NAME_CAP],
            unsigned texture_count,cnk_kart_skin *slot,unsigned lod,
            unsigned seen[CNK_KART_MAX_TEXTURES]){
 cnk_material_reference refs[CNK_KART_MAX_SELECTORS]={{0}};
 unsigned groups=0;
 if(cnk_model_material_from_vfs(vfs,idx,refs,CNK_KART_MAX_SELECTORS,&groups)||
    !groups)return -1;
 for(unsigned g=0;g<groups;g++){
  if(!refs[g].original_selector)continue;
  unsigned j=0;
  for(;j<texture_count;j++)if(!strcmp(refs[g].source_png,names[j]))break;
  if(j==texture_count)continue; /* unreviewed material remains untextured */
  unsigned t;
  for(t=0;t<slot->material_count[lod];t++)
   if(slot->materials[lod][t].selector==refs[g].original_selector)break;
  if(t!=slot->material_count[lod]){
   if(slot->materials[lod][t].texture_index!=j)return -1;
  }else{
   if(t>=CNK_KART_MAX_SELECTORS)return -1;
   slot->materials[lod][t]=(cnk_kart_selector_binding){refs[g].original_selector,j};
   slot->material_count[lod]++;
  }
  seen[j]++;
 }
 return 0;
}
int cnk_kart_skins_load(const char *path,const fufs_file *vfs,
     const cnk_png_catalog *cat,const cnk_racer_assets *racers,
     cnk_kart_skins *out){
 if(!out||!path||!*path||!vfs||!vfs->data||!vfs->entries||!cat||!racers)return -1;
 memset(out,0,sizeof(*out));
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 typedef struct {unsigned png_index;uint32_t png_id;
                 char name[CNK_TEXL_NAME_CAP];} texture_spec;
 typedef struct {unsigned count,levels;
                const cnk_object_mesh *model[3];
                texture_spec textures[CNK_KART_MAX_TEXTURES];} spec;
 spec entries[CNK_MAX_RACERS]={{0}};
 char line[1024];int rc=0;
 while(fgets(line,sizeof(line),fp)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1 && line[n-1]!='\n'){rc=-4;break;}
  while(n&&(line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
  if(!n||line[0]=='#')continue;
  char *fields[4]={line};unsigned nf=1;
  for(char *p=line;*p;p++)if(*p=='\t'){
   if(nf==4){rc=-5;break;}
   *p=0;fields[nf++]=p+1;
  }
  if(rc)break;
  unsigned slot,index;uint32_t id;
  if(nf!=4||dec(fields[0],&slot)||slot>=CNK_MAX_RACERS||
     dec(fields[2],&index)||hex(fields[3],&id)||
     !*fields[1]||strlen(fields[1])>=CNK_TEXL_NAME_CAP||
     !racers->assigned[slot]||index>=vfs->count||
     vfs->entries[index].id!=id){rc=-5;break;}
  const cnk_png_resource *png=cnk_png_find_index(cat,index);
  if(!png||png->vfs_id!=id){rc=-6;break;}
  spec *e=&entries[slot];
  if(e->count==CNK_KART_MAX_TEXTURES){rc=-5;break;}
  for(unsigned i=0;i<e->count;i++)
   if(!strcmp(e->textures[i].name,fields[1])){rc=-5;break;}
  if(rc)break;
  if(!e->count){
   if(racers->has_lod[slot]){
    e->levels=3;
    for(unsigned k=0;k<3;k++)e->model[k]=&racers->lods[slot].levels[k];
   }else{
    e->levels=1;e->model[0]=&racers->meshes[slot];
   }
  }
  texture_spec *t=&e->textures[e->count];
  t->png_index=index;t->png_id=id;
  memcpy(t->name,fields[1],strlen(fields[1])+1);
  e->count++;
 }
 if(ferror(fp))rc=-8;
 fclose(fp);
 if(rc)return rc;
 cnk_kart_skins stage={0};
 /* Validate ALL model/material selectors before uploading ANY GPU texture. */
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(entries[i].count){
  spec *e=&entries[i];cnk_kart_skin *slot=&stage.slots[i];
  slot->levels=e->levels;slot->texture_count=e->count;
  char names[CNK_KART_MAX_TEXTURES][CNK_TEXL_NAME_CAP]={{0}};
  for(unsigned j=0;j<e->count;j++)
   memcpy(names[j],e->textures[j].name,strlen(e->textures[j].name)+1);
  unsigned total[CNK_KART_MAX_TEXTURES]={0};
  for(unsigned k=0;k<e->levels;k++){
   uint32_t model_idx=racers->has_lod[i]?
      racers->lods[i].source[k].vfs_index:racers->index[i];
   unsigned found[CNK_KART_MAX_TEXTURES]={0};
   if(collect_materials(vfs,model_idx,names,e->count,slot,k,found) || !found[0]){
    cnk_kart_skins_free(&stage);return -7;
   }
   slot->models[k]=e->model[k];
   for(unsigned j=0;j<e->count;j++)total[j]+=found[j];
  }
  for(unsigned j=0;j<e->count;j++)if(!total[j]){
   cnk_kart_skins_free(&stage);return -7;
  }
 }
 /* All source references are now validated. Upload the minimum textures:
  * one handle per source PNG, shared by every mesh LOD in that racer slot. */
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(entries[i].count){
  spec *e=&entries[i];cnk_kart_skin *slot=&stage.slots[i];
  if(cnk_vitagl_textures_init(&slot->textures,vfs,cat,e->count)){
   cnk_kart_skins_free(&stage);return -9;
  }
  for(unsigned k=0;k<e->count;k++)
   if(cnk_vitagl_textures_upload(&slot->textures,
          (cnk_texture_binding){(uint16_t)k,(uint32_t)e->textures[k].png_index})){
    cnk_kart_skins_free(&stage);return -9;
   }
  slot->active=1;stage.count++;
 }
 *out=stage;
 return 0;
}
int cnk_kart_skins_apply(cnk_kart_skins *s,
     cnk_racer_visual_attachment a[CNK_MAX_RACERS]){
 if(!s||!a)return -1;
 unsigned selected[CNK_MAX_RACERS]={0};
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(s->slots[i].active){
  cnk_kart_skin *v=&s->slots[i];
  if(!a[i].mesh||a[i].resolve_material||a[i].material_context)return -2;
  unsigned k;
  for(k=0;k<v->levels;k++)if(v->models[k]==a[i].mesh)break;
  if(k==v->levels)return -3;
  selected[i]=k;
 }
 /* All checks completed before modifying the caller's attachments. */
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(s->slots[i].active){
  s->slots[i].current_level=selected[i];
  a[i].resolve_material=cnk_kart_skins_resolve;
  a[i].material_context=&s->slots[i];
 }
 return 0;
}
