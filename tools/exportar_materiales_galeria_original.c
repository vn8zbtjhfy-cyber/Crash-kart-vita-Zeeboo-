#include "fufs.h"
#include "cnk_source_kart_catalog.h"
#include "cnk_model_material.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* Source-only audit: prints every material POLY selector and original TEXL
 * filename from each source 3DOB, without guessing the corresponding PNG ID. */
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"Usage: %s data.vfs\n",argv[0]);return 2;}
 fufs_file vfs={0};
 if(fufs_open(argv[1],&vfs)){fprintf(stderr,"FUFS open failed\n");return 3;}
 printf("family_0based\tlod_0_high\tmodel_index\tmodel_id\tpoly_group\tselector\ttexl_name\tpng_identity\n");
 unsigned rows=0;
 for(unsigned family=0;family<6;family++)for(unsigned lod=0;lod<3;lod++){
  uint32_t idx=0,id=0;unsigned count=0;
  cnk_material_reference refs[512]={{0}};
  if(cnk_source_kart_gallery_source(family,lod,&idx,&id)||idx>=vfs.count||
     vfs.entries[idx].id!=id||
     cnk_model_material_from_vfs(&vfs,idx,refs,512,&count)||!count){
   fprintf(stderr,"Invalid source model family=%u lod=%u\n",family,lod);
   fufs_close(&vfs);return 4;
  }
  for(unsigned j=0;j<count;j++){
   /* Original TEXL names are checked as exact strings; missing names remain
    * empty rather than receiving invented PNG resource identities. */
   if(strchr(refs[j].source_png,'\t')||strchr(refs[j].source_png,'\n')){
    fufs_close(&vfs);return 5;
   }
   printf("%u\t%u\t%u\t%08x\t%u\t%u\t%s\tNO_VERIFICADO\n",
      family,lod,idx,id,j,refs[j].original_selector,refs[j].source_png);
   rows++;
  }
 }
 fufs_close(&vfs);
 if(rows<18)return 6;
 return 0;
}
