#include "cnk_pad_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
static int valid_id(const char *s,uint32_t *id){
 if(!s||!*s||*s=='-'||*s=='+')return -1;
 char *end=NULL;errno=0;
 unsigned long v=strtoul(s,&end,0);
 if(errno||!end||*end||!v||v>UINT32_MAX)return -1;
 *id=(uint32_t)v;return 0;
}
static int valid_float(const char *s,float *value){
 if(!s||!*s)return -1;
 char *end=NULL;errno=0;
 float n=strtof(s,&end);
 if(errno||!end||*end||!isfinite(n)||fabsf(n)>100000.f)return -1;
 *value=n;return 0;
}
int cnk_pad_layout_read(const char *path,cnk_pad_layout *out){
 if(!path||!*path||!out)return -1;
 FILE *f=fopen(path,"rb");
 if(!f)return errno==ENOENT?-2:-3;
 cnk_pad_layout staged={0};
 char line[320];int rc=0;
 while(fgets(line,sizeof line,f)){
  size_t len=strlen(line);
  if(len==sizeof(line)-1&&line[len-1]!='\n'){rc=-4;break;}
  char *p=line;
  while(*p==' '||*p=='\t')p++;
  if(*p=='#'||*p=='\n'||*p=='\r'||!*p)continue;
  if(staged.count==CNK_PAD_LAYOUT_MAX){rc=-5;break;}
  char *parts[6]={0};unsigned count=0;
  for(char *tok=strtok(p,"\t\r\n");tok;tok=strtok(NULL,"\t\r\n")){
   if(count==6)break;
   parts[count++]=tok;
  }
  if(count!=5){rc=-6;break;}
  cnk_pad_layout_entry e={0};
  if(valid_id(parts[0],&e.scene_resource_id)||
     valid_float(parts[1],&e.position.x)||
     valid_float(parts[2],&e.position.y)||
     valid_float(parts[3],&e.position.z)||
     valid_float(parts[4],&e.radius)) {rc=-7;break;}
  cnk_boost_pad_world validation={0};
  if(cnk_boost_pad_place(&validation,e.position,e.radius)){rc=-8;break;}
  staged.entries[staged.count++]=e;
 }
 int read_error=ferror(f);
 int close_error=fclose(f);
 if(read_error||close_error)return -9;
 if(rc)return rc;
 *out=staged;
 return 0;
}
int cnk_pad_layout_apply(const cnk_pad_layout *layout,cnk_game *g){
 if(!layout||!g||!g->loaded||!g->vfs||
    g->scene_index>=g->vfs->count||layout->count>CNK_PAD_LAYOUT_MAX)return -1;
 uint32_t id=g->vfs->entries[g->scene_index].id;
 cnk_boost_pad_world next=g->session.pads;
 int found=0;
 for(uint32_t i=0;i<layout->count;i++){
  const cnk_pad_layout_entry *entry=&layout->entries[i];
  if(entry->scene_resource_id!=id)continue;
  if(cnk_boost_pad_place(&next,entry->position,entry->radius))return -2;
  found++;
 }
 g->session.pads=next;
 return found;
}
