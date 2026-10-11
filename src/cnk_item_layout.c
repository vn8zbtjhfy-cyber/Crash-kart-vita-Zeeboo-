#include "cnk_item_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

static int parse_u32(const char *s,uint32_t *out){
 char *end=NULL;
 if(!s||!*s||*s=='-'||*s=='+')return -1;
 errno=0;unsigned long n=strtoul(s,&end,0);
 if(errno||!end||*end||n>UINT32_MAX)return -1;
 *out=(uint32_t)n;return 0;
}
static int parse_float(const char *s,float *out){
 char *end=NULL;
 if(!s||!*s)return -1;
 errno=0;float n=strtof(s,&end);
 if(errno||!end||*end||!isfinite(n)||fabsf(n)>100000)return -1;
 *out=n;return 0;
}
static int parse_line(char *line,cnk_item_layout_entry *out){
 char *cols[8]={0};int count=0;
 for(char *part=strtok(line,"\t\r\n");part;part=strtok(NULL,"\t\r\n")){
  if(count>=8)return -1;
  cols[count++]=part;
 }
 if(count!=7)return -1;
 cnk_item_layout_entry e={0};
 if(parse_u32(cols[0],&e.scene_resource_id)||!e.scene_resource_id ||
    parse_float(cols[2],&e.pickup.position.x)||
    parse_float(cols[3],&e.pickup.position.y)||
    parse_float(cols[4],&e.pickup.position.z)||
    parse_float(cols[5],&e.pickup.radius))return -1;
 if(!strcmp(cols[1],"WUMPA"))e.pickup.kind=CNK_PICKUP_WUMPA;
 else if(!strcmp(cols[1],"CRATE"))e.pickup.kind=CNK_PICKUP_CRATE;
 else return -1;
 if(!strcmp(cols[6],"NONE"))e.pickup.reward=CNK_WEAPON_NONE;
 else if(!strcmp(cols[6],"TURBO"))e.pickup.reward=CNK_WEAPON_TURBO;
 else if(!strcmp(cols[6],"BOMB"))e.pickup.reward=CNK_WEAPON_BOMB;
 else if(!strcmp(cols[6],"MISSILE"))e.pickup.reward=CNK_WEAPON_MISSILE;
 else if(!strcmp(cols[6],"TNT"))e.pickup.reward=CNK_WEAPON_TNT;
 else if(!strcmp(cols[6],"OIL"))e.pickup.reward=CNK_WEAPON_OIL;
 else if(!strcmp(cols[6],"SHIELD"))e.pickup.reward=CNK_WEAPON_SHIELD;
 else if(!strcmp(cols[6],"TROPY_CLOCK"))e.pickup.reward=CNK_WEAPON_TROPY_CLOCK;
 else if(!strcmp(cols[6],"CURSED_CANDLE"))e.pickup.reward=CNK_WEAPON_CURSED_CANDLE;
 else if(!strcmp(cols[6],"AKU_AKU"))e.pickup.reward=CNK_WEAPON_AKU_AKU;
 else if(!strcmp(cols[6],"KART_JACKING"))e.pickup.reward=CNK_WEAPON_KART_JACKING;
 else return -1;
 /* Reuse exact game-world validation with no side effects on the caller. */
 cnk_item_world validation={0};
 if(cnk_items_place(&validation,e.pickup.kind,e.pickup.position,
                    e.pickup.radius,e.pickup.reward))return -1;
 *out=e;return 0;
}
int cnk_item_layout_read(const char *path,cnk_item_layout *out){
 if(!out||!path||!*path)return -1;
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 cnk_item_layout parsed={0};
 char line[320];int rc=0;
 while(fgets(line,sizeof(line),fp)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1 && line[n-1]!='\n'){rc=-4;break;}
  char *first=line;while(*first==' '||*first=='\t')first++;
  if(*first=='#'||*first=='\n'||*first=='\r'||!*first)continue;
  if(parsed.count==CNK_ITEM_LAYOUT_MAX){rc=-5;break;}
  if(parse_line(first,&parsed.entries[parsed.count])){rc=-6;break;}
  parsed.count++;
 }
 if(ferror(fp))rc=-7;
 if(fclose(fp))rc=-7;
 if(rc)return rc;
 *out=parsed;return 0;
}
int cnk_item_layout_apply(const cnk_item_layout *layout,cnk_game *g){
 if(!layout||!g||!g->loaded||!g->vfs||
    g->scene_index>=g->vfs->count||layout->count>CNK_ITEM_LAYOUT_MAX)return -1;
 uint32_t resource_id=g->vfs->entries[g->scene_index].id;
 cnk_item_world next=g->session.items;
 int matches=0;
 for(uint32_t i=0;i<layout->count;i++){
  const cnk_item_layout_entry *e=&layout->entries[i];
  if(e->scene_resource_id!=resource_id)continue;
  const cnk_item_pickup *p=&e->pickup;
  if(cnk_items_place(&next,p->kind,p->position,p->radius,p->reward))return -2;
  matches++;
 }
 g->session.items=next;
 return matches;
}
