#include "cnk_collision_probe.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
int cnk_collision_probe_read(const char *path,cnk_collision_probe *out){
 if(!path||!*path||!out)return -1;
 FILE *f=fopen(path,"rb");
 if(!f)return errno==ENOENT?-2:-3;
 char line[128];unsigned seen=0;int rc=0;
 cnk_collision_probe value={0};
 while(fgets(line,sizeof(line),f)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1 && line[n-1]!='\n'){rc=-4;break;}
  while(n && (line[n-1]=='\r'||line[n-1]=='\n'))line[--n]=0;
  if(!n||line[0]=='#')continue;
  if(!strcmp(line,"WALLS\t0") || !strcmp(line,"WALLS\t1")){
   if(seen&1u){rc=-5;break;}
   value.walls=line[6]-'0';seen|=1u;
  }else if(!strcmp(line,"KARTS\t0") || !strcmp(line,"KARTS\t1")){
   if(seen&2u){rc=-5;break;}
   value.karts=line[6]-'0';seen|=2u;
  }else{rc=-5;break;}
 }
 if(ferror(f))rc=-6;
 if(fclose(f)!=0 && !rc)rc=-6;
 if(!rc && seen!=3u)rc=-5;
 if(!rc)*out=value;
 return rc;
}
