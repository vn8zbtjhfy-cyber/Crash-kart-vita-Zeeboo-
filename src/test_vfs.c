#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
int main(int argc,char **argv){
  fufs_file f;int ret,i;unsigned char *bytes;size_t len;unsigned counts[8]={0};
  const char *types[]={"png","piff","wav","mp3","pfnt","lang","form","bin"};
  if(argc<2||argc>3){fprintf(stderr,"uso: %s data.vfs [directorio_descargas]\n",argv[0]);return 2;}
  if((ret=fufs_open(argv[1],&f))){fprintf(stderr,"FUFS error: %d\n",ret);return 1;}
  printf("archivos: %u\n",f.count);
  for(i=0;i<(int)f.count;i++){
    FILE *h;char name[512];unsigned j;
    ret=fufs_extract(&f,(uint32_t)i,&bytes,&len);
    if(ret){fprintf(stderr,"error recurso %d: %d\n",i,ret);fufs_close(&f);return 1;}
    for(j=0;j<8;j++)if(!strcmp(fufs_type(bytes,len),types[j])){counts[j]++;break;}
    printf("%03d %08x %s %zu %08lx\n",i,f.entries[i].id,fufs_type(bytes,len),len,(unsigned long)crc32(0,bytes,(uInt)len));
    if(argc==3){snprintf(name,sizeof(name),"%s/%03d_%08x.%s",argv[2],i,f.entries[i].id,fufs_type(bytes,len));
      h=fopen(name,"wb");if(!h){free(bytes);fufs_close(&f);return 3;}
      if(fwrite(bytes,1,len,h)!=len){fclose(h);free(bytes);fufs_close(&f);return 4;}fclose(h);
    }
    free(bytes);
  }
  for(i=0;i<8;i++)printf("%s: %u\n",types[i],counts[i]);
  fufs_close(&f);return 0;
}
