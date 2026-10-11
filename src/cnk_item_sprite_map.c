#include "cnk_item_sprite_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
static int item_name(const char *s){
 static const char *names[]={"WUMPA","CRATE","BOMB","MISSILE","TNT","OIL"};
 for(int i=0;i<CNK_ITEM_SPRITE_COUNT;i++)if(!strcmp(s,names[i]))return i;
 return -1;
}
int cnk_item_sprite_map_read(const char *path,const fufs_file *vfs,
                             const cnk_png_catalog *catalog,cnk_item_sprite_map *out){
 if(!path||!vfs||!catalog||!out||!vfs->entries)return -1;
 FILE *f=fopen(path,"rb");if(!f)return -2;
 char line[256];cnk_item_sprite_map tmp={0};int status=0;
 while(fgets(line,sizeof(line),f)){
  if(!strchr(line,'\n')&&!feof(f)){status=-3;break;}
  char *p=line;while(*p==' '||*p=='\t')p++;
  if(!*p||*p=='#'||*p=='\r'||*p=='\n')continue;
  char name[32]={0},index_str[32]={0},id_str[32]={0},extra[2]={0};
  if(sscanf(p,"%31s %31s %31s %1s",name,index_str,id_str,extra)!=3){status=-3;break;}
  int sprite=item_name(name);
  if(sprite<0||tmp.present[sprite]){status=-3;break;}
  for(char *q=index_str;*q;q++)if(!isdigit((unsigned char)*q)){status=-3;break;}
  if(status)break;
  char *end=NULL;unsigned long idx=strtoul(index_str,&end,10);
  if(!end||*end||idx>=vfs->count||idx>UINT32_MAX){status=-3;break;}
  if(strlen(id_str)!=8){status=-3;break;}
  for(char *q=id_str;*q;q++)if(!isxdigit((unsigned char)*q)){status=-3;break;}
  if(status)break;
  unsigned long id=strtoul(id_str,&end,16);
  const cnk_png_resource *image=cnk_png_find_index(catalog,(uint32_t)idx);
  if(!end||*end||!image||image->vfs_id!=(uint32_t)id||
     vfs->entries[idx].id!=(uint32_t)id){status=-4;break;}
  tmp.present[sprite]=1;
  tmp.binding[sprite]=(cnk_texture_binding){(uint16_t)sprite,(uint32_t)idx};
  tmp.vfs_id[sprite]=(uint32_t)id;tmp.count++;
 }
 if(ferror(f))status=-5;
 if(fclose(f))status=-5;
 if(status)return status;
 *out=tmp;return 0;
}
