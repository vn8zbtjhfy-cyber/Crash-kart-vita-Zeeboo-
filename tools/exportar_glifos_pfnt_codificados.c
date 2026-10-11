/* Audits/source-extracts original PFNT glyph byte rectangles. Not a decoder. */
#include "fufs.h"
#include "cnk_pfnt.h"
#include "cnk_pfnt_glyph_data.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <zlib.h>

static int read_u32(const char *s,uint32_t *out){
  unsigned long x;char *end=NULL;
  if(!s||!s[0]||s[0]=='-'||s[0]=='+')return -1;
  errno=0;x=strtoul(s,&end,10);
  if(errno||!end||*end||x>UINT32_MAX)return -1;
  *out=(uint32_t)x;return 0;
}
int main(int argc,char **argv){
  fufs_file f={0};uint32_t chosen=0,cp=0;int extraction=(argc==6),fonts=0;
  if(argc!=2&&!extraction){fprintf(stderr,"Usage: %s data.vfs [--extract index decimal_codepoint output.bin]\n",argv[0]);return 2;}
  if(extraction&&(strcmp(argv[2],"--extract")||read_u32(argv[3],&chosen)||read_u32(argv[4],&cp)))return 2;
  /* A source archive must never be overwritten by the extractor, even if
   * the destination is a different pathname or hard link to the same file. */
  if(extraction){
    struct stat source, destination;
    if(stat(argv[1],&source)!=0)return 3;
    if(stat(argv[5],&destination)==0 &&
       source.st_dev==destination.st_dev && source.st_ino==destination.st_ino)return 2;
  }
  if(fufs_open(argv[1],&f)!=0)return 3;
  if(extraction&&chosen>=f.count){fufs_close(&f);return 4;}
  if(!extraction)puts("vfs_index\tfufs_id_hex\tcodepoint\tatlas_x\twidth_bytes\trows\tencoded_bytes\tcrc32");
  for(uint32_t i=0;i<f.count;i++){
    uint8_t *blob=NULL,*raw=NULL;size_t len=0;
    cnk_pfnt_view font={0};int status=0;
    if(extraction&&i!=chosen)continue;
    if(fufs_extract(&f,i,&blob,&len)!=0){fufs_close(&f);return 5;}
    if(strcmp(fufs_type(blob,len),"pfnt")!=0){free(blob);if(extraction){fufs_close(&f);return 6;}continue;}
    fonts++;
    if(cnk_pfnt_open(blob,len,&font)!=0){free(blob);fufs_close(&f);return 7;}
    for(uint32_t j=0;j<font.glyph_count;j++){
      cnk_pfnt_encoded_glyph g={0};size_t bytes;uLong crc;
      uint16_t codepoint=(uint16_t)(blob[18u+j*2u] | (uint16_t)(blob[19u+j*2u]<<8));
      if(extraction&&codepoint!=cp)continue;
      if(cnk_pfnt_encoded_glyph_get(&font,codepoint,&g)!=0){status=8;break;}
      bytes=cnk_pfnt_encoded_glyph_bytes(&g);
      raw=(uint8_t*)malloc(bytes?bytes:1u);
      if(!raw||cnk_pfnt_encoded_glyph_copy(&g,raw,bytes)!=0){status=9;break;}
      if(extraction){
        FILE *out=fopen(argv[5],"wb");
        if(!out){status=10;break;}
        size_t written=fwrite(raw,1,bytes,out);
        int closed=fclose(out);
        if(written!=bytes||closed!=0){status=11;break;}
        free(raw);free(blob);fufs_close(&f);return 0;
      }
      crc=crc32(0L,Z_NULL,0);crc=crc32(crc,raw,(uInt)bytes);
      printf("%u\t%08x\t%u\t%u\t%u\t%u\t%zu\t%08lx\n",
             i,f.entries[i].id,codepoint,g.atlas_x_bytes,g.width_bytes,g.rows,bytes,(unsigned long)crc);
      free(raw);raw=NULL;
    }
    free(raw);free(blob);
    if(status){fufs_close(&f);return status;}
  }
  fufs_close(&f);
  return extraction?12:(fonts==5?0:13);
}
