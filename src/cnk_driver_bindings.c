#include "cnk_driver_bindings.h"
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int decimal_slot(const char *s,unsigned *out){
 if(!s||!s[0]||s[1]||s[0]<'0'||s[0]>'9')return -1;
 *out=(unsigned)(s[0]-'0');return *out<CNK_MAX_RACERS?0:-1;
}
static int character_id(const char *s,cnk_character_identity *out){
 if(!strcmp(s,"crash"))*out=CNK_CHARACTER_CRASH;
 else if(!strcmp(s,"coco"))*out=CNK_CHARACTER_COCO;
 else if(!strcmp(s,"ripper_roo"))*out=CNK_CHARACTER_RIPPER_ROO;
 else return -1;
 return 0;
}
static int parse_transform(char *const *s,float m[16]){
 for(unsigned i=0;i<16;i++){
  char *end=NULL;errno=0;
  if(!s[i]||!*s[i])return -1;
  m[i]=strtof(s[i],&end);
  if(errno==ERANGE||!end||*end||!isfinite(m[i]))return -1;
 }
 if(fabsf(m[3])>1e-6f||fabsf(m[7])>1e-6f||fabsf(m[11])>1e-6f||
    fabsf(m[15]-1.0f)>1e-6f)return -1;
 double det=(double)m[0]*(m[5]*m[10]-m[9]*m[6])-
            (double)m[4]*(m[1]*m[10]-m[9]*m[2])+
            (double)m[8]*(m[1]*m[6]-m[5]*m[2]);
 if(!isfinite(det)||fabs(det)<1e-8)return -1;
 return 0;
}
int cnk_driver_bindings_load(const char *path,cnk_driver_bindings *out){
 if(!path||!*path||!out)return -1;
 memset(out,0,sizeof(*out));
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 cnk_driver_bindings stage={0};
 char line[2048];int rc=0;
 while(fgets(line,sizeof(line),fp)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1&&line[n-1]!='\n'){rc=-4;break;}
  while(n&&(line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;
  if(!n||line[0]=='#')continue;
  char *cols[18];unsigned nc=1;cols[0]=line;
  for(char *p=line;*p;p++)if(*p=='\t'){
   if(nc==18){rc=-5;break;}
   *p=0;cols[nc++]=p+1;
  }
  if(rc)break;
  unsigned slot;cnk_character_identity who;
  if(nc!=18||decimal_slot(cols[0],&slot)||character_id(cols[1],&who)||
     stage.assigned[slot]||parse_transform(&cols[2],stage.calibration[slot])){
   rc=-5;break;
  }
  stage.assigned[slot]=1;
  stage.identity[slot]=who;
  stage.count++;
 }
 if(ferror(fp))rc=-6;
 fclose(fp);
 if(rc)return rc;
 *out=stage;
 return 0;
}
int cnk_driver_bindings_attachments(const cnk_driver_bindings *b,
   const cnk_character_pack *pack,const uint8_t kart_slots[CNK_MAX_RACERS],
   cnk_racer_visual_attachment out[CNK_MAX_RACERS]){
 if(!b||!pack||!kart_slots||!out||!pack->ready)return -1;
 cnk_racer_visual_attachment stage[CNK_MAX_RACERS]={{0}};
 unsigned count=0;
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(b->assigned[i]){
  if(!kart_slots[i]||b->identity[i]<0||b->identity[i]>=CNK_CHARACTER_COUNT)return -2;
  const cnk_character_asset *character=&pack->characters[b->identity[i]];
  if(!character->skin.ready)return -3;
  stage[i].mesh=&character->skin.mesh;
  stage[i].local_from_mesh=b->calibration[i];
  stage[i].resolve_material=cnk_character_original_resolve;
  stage[i].material_context=(void *)character;
  if(cnk_racer_visual_attachment_valid(&stage[i]))return -4;
  count++;
 }
 if(count!=b->count)return -5;
 memcpy(out,stage,sizeof(stage));
 return 0;
}
