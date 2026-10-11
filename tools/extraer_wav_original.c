/* Host-only WAV evidence extractor. Requires BOTH FUFS index and source ID.
 * Bytes are extracted unchanged; never guesses the historical game event. */
#include "fufs.h"
#include "cnk_audio.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
static int number(const char *s,unsigned base,uint32_t *out){
 char *end=NULL;unsigned long value;
 if(!s||!*s||*s=='-'||*s=='+')return -1;
 errno=0;value=strtoul(s,&end,base);
 if(errno||!end||*end||value>UINT32_MAX)return -1;
 *out=(uint32_t)value;return 0;
}
int main(int argc,char **argv){
 if(argc!=5){fprintf(stderr,"Usage: %s data.vfs FUFS_INDEX FUFS_ID_HEX target.wav\n",argv[0]);return 2;}
 /* Avoid replacing the source ROM itself, including same-file aliases. */
 struct stat original,requested;
 if(stat(argv[1],&original)==0 && stat(argv[4],&requested)==0 &&
    original.st_dev==requested.st_dev && original.st_ino==requested.st_ino){
  fputs("Cannot overwrite source archive\n",stderr);return 10;
 }
 uint32_t index=0,id=0;
 if(number(argv[2],10,&index)||number(argv[3],16,&id)){fputs("Invalid index or source ID\n",stderr);return 3;}
 fufs_file vfs={0};
 if(fufs_open(argv[1],&vfs))return 4;
 if(index>=vfs.count||vfs.entries[index].id!=id){fufs_close(&vfs);return 5;}
 unsigned char *bytes=NULL;size_t len=0;
 if(fufs_extract(&vfs,index,&bytes,&len)){fufs_close(&vfs);return 6;}
 cnk_wav pcm={0};
 if(cnk_wav_open(bytes,len,&pcm)){free(bytes);fufs_close(&vfs);return 7;}
 /* Fail before touching output when resource validation is unsuccessful. */
 FILE *out=fopen(argv[4],"wb");
 if(!out){free(bytes);fufs_close(&vfs);return 8;}
 int ok=(fwrite(bytes,1,len,out)==len);
 if(fclose(out)!=0)ok=0;
 /* Size and decoded format are a matter of original data, not opinion. */
 free(bytes);fufs_close(&vfs);
 if(!ok){remove(argv[4]);return 9;}
 return 0;
}
