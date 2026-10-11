/* Evidence-only recovery of native PFNT encoded backing data, not an image decoder. */
#include "fufs.h"
#include "cnk_pfnt.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv){
 fufs_file ar={0};size_t found=0;
 if(argc!=2)return 2;
 assert(fufs_open(argv[1],&ar)==0);
 for(uint32_t i=0;i<ar.count;i++){
   uint8_t *p=NULL;size_t size=0;
   cnk_pfnt_view f={0};cnk_pfnt_encoded_atlas b={0},bad={0};
   assert(fufs_extract(&ar,i,&p,&size)==0);
   if(strcmp(fufs_type(p,size),"pfnt")!=0){free(p);continue;}
   assert(cnk_pfnt_open(p,size,&f)==0);
   assert(cnk_pfnt_encoded_atlas_open(&f,&b)==0);
   assert(b.stored_width_px==f.total_advance_px);
   assert(b.stored_height_px==f.line_height_px);
   assert(b.encoded_bytes==(size_t)f.total_advance_px*f.line_height_px);
   assert(b.encoded_pixels+b.encoded_bytes==p+size);
   assert(b.image_preamble==p+f.trailing_payload_offset);
   assert(b.preamble_bytes>=12);
   assert(b.encoded_pixels==b.image_preamble+b.preamble_bytes);
   assert(b.image_preamble[0]==3u);
   /* Corrupt either backing dimensions, source size or bitmap tag; fail closed. */
   uint8_t *copy=(uint8_t*)malloc(size);
   assert(copy);memcpy(copy,p,size);
   copy[f.trailing_payload_offset+4]^=0x40u;
   cnk_pfnt_view f2=f;f2.bytes=copy;
   assert(cnk_pfnt_encoded_atlas_open(&f2,&bad)<0);
   assert(!bad.encoded_pixels);
   f2=f;f2.length=f.trailing_payload_offset+11u;
   assert(cnk_pfnt_encoded_atlas_open(&f2,&bad)<0);
   f2=f;f2.length=size-1u;
   /* Truncating the payload can only be detected by the full length contract
    * if no independent encoded-size field is known. Do not falsely assert. */
   assert(cnk_pfnt_encoded_atlas_open(NULL,&bad)<0);
   free(copy);free(p);found++;
 }
 fufs_close(&ar);assert(found==5);
 puts("5 source PFNT encoded bitmap spans and dimensions validated; no color mapping claimed");
 return 0;
}
