#include "cnk_cup_manifest.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/* Small bounded parser. Never reads arbitrary-size untrusted app0 files into
 * game memory, never writes into VFS, and accepts decimal integers only. */
static int number(char **cursor,uint32_t *out){
 char *p=*cursor;
 while(*p==' '||*p=='\t')p++;
 if(!isdigit((unsigned char)*p))return -1;
 uint64_t v=0;
 do {
  uint32_t digit=(uint32_t)(*p-'0');
  if(v>(UINT32_MAX-digit)/10u)return -1;
  v=v*10u+digit;
  p++;
 }while(isdigit((unsigned char)*p));
 if(*p && *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && *p!='#')return -1;
 *cursor=p;*out=(uint32_t)v;return 0;
}
static int finish(char *p){
 while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;
 return *p==0 || *p=='#';
}
int cnk_cup_manifest_read(const char *path,const cnk_track_catalog *catalog,
                          uint32_t racer_count,cnk_cup_manifest *out){
 if(!path||!path[0]||!catalog||!catalog->source||!out||
    racer_count<2||racer_count>CNK_MAX_RACERS)return -1;
 FILE *f=fopen(path,"rb");if(!f)return errno==ENOENT?-2:-3;
 /* Reject embedded NULs and oversized binary input before any text parse.
  * fgets/strlen alone could otherwise hide bytes after an interior NUL. */
 size_t total_bytes=0;
 int ch=0,precheck=0;
 while((ch=fgetc(f))!=EOF){
  if(!ch || ++total_bytes>16384u){precheck=-4;break;}
 }
 if(ferror(f))precheck=-3;
 if(precheck || fseek(f,0,SEEK_SET)){
  fclose(f);return precheck?precheck:-3;
 }
 cnk_cup_manifest v={0};v.racer_count=racer_count;
 int header=0, points=0, error=0;
 char line[256];uint32_t count=0;
 while(fgets(line,sizeof(line),f)){
  if(++count>64){error=-4;break;}
  size_t len=strlen(line);
  if(len==sizeof(line)-1 && line[len-1]!='\n' && !feof(f)){
   error=-4;break;
  }
  char *p=line;
  while(*p==' '||*p=='\t')p++;
  if(*p=='#'||*p=='\r'||*p=='\n'||!*p)continue;
  if(!header){
   if(strncmp(p,"CNK_CUP_DEV_V1",14) || !finish(p+14)){
    error=-5;break;
   }
   header=1;continue;
  }
  if(!strncmp(p,"TRACK",5) && (p[5]==' '||p[5]=='\t')){
   if(points || v.round_count>=CNK_CUP_MAX_ROUNDS){error=-6;break;}
   char *cursor=p+5;
   uint32_t id=0;
   if(number(&cursor,&id)||!id||!finish(cursor)){error=-6;break;}
   v.resource_ids[v.round_count++]=id;
  }else if(!strncmp(p,"POINTS",6) && (p[6]==' '||p[6]=='\t')){
   if(points || v.round_count<2){error=-7;break;}
   char *cursor=p+6;
   for(uint32_t i=0;i<racer_count;i++)
    if(number(&cursor,&v.points[i])){error=-7;break;}
   if(error)break;
   if(!finish(cursor)){error=-7;break;}
   points=1;
  }else {error=-8;break;}
 }
 if(ferror(f))error=-9;
 if(fclose(f)!=0)error=-9;
 if(error)return error;
 if(!header||!points||v.round_count<2)return -10;
 cnk_cup cup={0};
 if(cnk_cup_init(&cup,catalog,v.resource_ids,v.round_count,
                 v.points,v.racer_count))return -11;
 *out=v;return 0;
}
