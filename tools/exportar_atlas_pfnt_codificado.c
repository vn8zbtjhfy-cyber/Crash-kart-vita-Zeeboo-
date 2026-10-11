/* Exact PFNT encoded backing data audit; not a rendering/decompression tool.
 * Uses no guessed palette, channel mapping, orientation or glyph alpha.
 */
#include "fufs.h"
#include "cnk_pfnt.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int write_exact(const char *file,const uint8_t *p,size_t length){
 FILE *f=fopen(file,"wb");size_t written=0;
 if(!f)return -1;
 written=fwrite(p,1,length,f);
 if(fclose(f)!=0 || written!=length)return -1;
 return 0;
}
int main(int argc,char **argv){
 fufs_file ar={0};int extracted=0;uint32_t chosen=UINT32_MAX;
 if(argc!=2 && argc!=5){
  fprintf(stderr,"Usage: %s data.vfs [--extract index encoded.raw]\n",argv[0]);return 2;
 }
 if(argc==5){
  char *end=NULL;unsigned long v;
  if(strcmp(argv[2],"--extract"))return 2;
  v=strtoul(argv[3],&end,10);
  if(!argv[3][0]||!end||*end||v>=UINT32_MAX)return 2;
  chosen=(uint32_t)v;
 }
 if(fufs_open(argv[1],&ar)!=0)return 3;
 if(chosen!=UINT32_MAX && chosen>=ar.count){fufs_close(&ar);return 4;}
 if(argc==2)puts("vfs_index\tfufs_id_hex\twidth_px\theight_px\tpreamble_bytes\tencoded_image_bytes\tencoded_image_offset\timage_tag");
 for(uint32_t i=0;i<ar.count;i++){
  uint8_t *blob=NULL;size_t size=0;
  cnk_pfnt_view font={0};cnk_pfnt_encoded_atlas bitmap={0};
  if(chosen!=UINT32_MAX && chosen!=i)continue;
  if(fufs_extract(&ar,i,&blob,&size)!=0){fufs_close(&ar);return 5;}
  if(strcmp(fufs_type(blob,size),"pfnt")){
   free(blob);
   if(chosen==i){fufs_close(&ar);return 6;}
   continue;
  }
  if(cnk_pfnt_open(blob,size,&font)!=0||cnk_pfnt_encoded_atlas_open(&font,&bitmap)!=0){
   free(blob);fufs_close(&ar);return 7;
  }
  if(chosen!=UINT32_MAX){
    int status=write_exact(argv[4],bitmap.encoded_pixels,bitmap.encoded_bytes);
    free(blob);fufs_close(&ar);
    return status?8:0;
  }
  printf("%u\t%08x\t%u\t%u\t%zu\t%zu\t%zu\t%u\n",i,ar.entries[i].id,
         bitmap.stored_width_px,bitmap.stored_height_px,bitmap.preamble_bytes,
         bitmap.encoded_bytes,(size_t)(bitmap.encoded_pixels-blob),
         bitmap.image_preamble[0]);
  extracted++;free(blob);
 }
 fufs_close(&ar);
 return extracted==5?0:9;
}
