#include "cnk_story_manifest.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

static int number(char **cursor,uint32_t *out){
 char *p=*cursor;
 while(*p==' '||*p=='\t')p++;
 if(!isdigit((unsigned char)*p))return -1;
 uint32_t v=0;
 do {
  uint32_t d=(uint32_t)(*p-'0');
  if(v>(UINT32_MAX-d)/10u)return -1;
  v=v*10u+d;p++;
 }while(isdigit((unsigned char)*p));
 if(*p && *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && *p!='#')return -1;
 *cursor=p;*out=v;return 0;
}
static int end_line(const char *p){
 while(*p==' '||*p=='\t'||*p=='\r'||*p=='\n')p++;
 return !*p||*p=='#';
}
int cnk_story_manifest_read(const char *path,const cnk_track_catalog *cat,
                            uint32_t racers,cnk_story_manifest *out){
 if(!path||!*path||!cat||!cat->source||!out||
    racers<2||racers>CNK_MAX_RACERS)return -1;
 FILE *f=fopen(path,"rb");if(!f)return errno==ENOENT?-2:-3;
 size_t bytes=0;int c=0,error=0;
 /* Reject hidden binary suffixes, even after syntactically valid header. */
 while((c=fgetc(f))!=EOF){
  if(!c||++bytes>8192u){error=-4;break;}
 }
 if(ferror(f))error=-3;
 if(!error && fseek(f,0,SEEK_SET))error=-3;
 cnk_story_manifest temp={0};
 unsigned rows=0,seen_header=0;
 char line[256];
 while(!error && fgets(line,sizeof(line),f)){
  if(++rows>64u){error=-5;break;}
  size_t len=strlen(line);
  if(len==sizeof(line)-1 && line[len-1]!='\n' && !feof(f)){
   error=-5;break;
  }
  char *p=line;while(*p==' '||*p=='\t')p++;
  if(!*p||*p=='#'||*p=='\n'||*p=='\r')continue;
  if(!seen_header){
   const char *hdr="CNK_STORY_DEV_V1";
   size_t length=strlen(hdr);
   if(strncmp(p,hdr,length)||!end_line(p+length)){error=-6;break;}
   seen_header=1;continue;
  }
  if(strncmp(p,"STAGE",5)||(p[5]!=' '&&p[5]!='\t')||
     temp.stage_count==CNK_STORY_MAX_STAGES){error=-7;break;}
  cnk_story_stage stage={0};char *cursor=p+5;
  if(number(&cursor,&stage.resource_id)||number(&cursor,&stage.laps)||
     number(&cursor,&stage.best_required_rank)||!end_line(cursor)){
   error=-7;break;
  }
  temp.stages[temp.stage_count++]=stage;
 }
 if(ferror(f))error=-3;
 if(fclose(f))error=-3;
 if(error)return error;
 if(!seen_header||!temp.stage_count)return -8;
 cnk_story verified={0};
 if(cnk_story_init(&verified,cat,temp.stages,temp.stage_count,racers,0))return -9;
 for(uint32_t i=0;i<temp.stage_count;i++)temp.stages[i]=verified.stages[i];
 *out=temp;return 0;
}
