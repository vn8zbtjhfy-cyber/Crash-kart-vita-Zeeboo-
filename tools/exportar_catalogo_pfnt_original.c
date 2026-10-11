/* Audits authentic PFNT character indices in Zeebo's FUFS ROM.
 * Exact PNG/bitmap drawing and historical UI assignments remain unknown. */
#include "fufs.h"
#include "cnk_pfnt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
  fufs_file archive={0};unsigned count=0;
  int glyph_mode=(argc==3 && strcmp(argv[2],"--glyphs")==0);
  if(!((argc==2)||(argc==3&&glyph_mode))){fprintf(stderr,"Usage: %s data.vfs [--glyphs]\n",argv[0]);return 2;}
  if(fufs_open(argv[1],&archive))return 3;
  if(glyph_mode)
    puts("vfs_index\tfufs_id_hex\tcodepoint_decimal\tatlas_x_px\tadvance_px");
  else
    puts("vfs_index\tfufs_id_hex\tfirst_codepoint\tlast_codepoint\tglyphs\tline_height_px\ttotal_advance_px\tencoded_bytes");
  for(uint32_t i=0;i<archive.count;i++){
    unsigned char *blob=NULL;size_t len=0;cnk_pfnt_view font;
    if(fufs_extract(&archive,i,&blob,&len)){fufs_close(&archive);return 4;}
    if(!strcmp(fufs_type(blob,len),"pfnt")){
      if(cnk_pfnt_open(blob,len,&font)){
        fprintf(stderr,"Corrupt PFNT index %u\n",i);free(blob);fufs_close(&archive);return 5;
      }
      if(glyph_mode){
        for(uint32_t cp=font.codepoint_first;cp<=font.codepoint_last;cp++){
          cnk_pfnt_glyph glyph={0};
          int found=cnk_pfnt_glyph_for(&font,cp,&glyph);
          if(found==0)printf("%u\t%08x\t%u\t%u\t%u\n",
              i,archive.entries[i].id,cp,glyph.atlas_x_px,glyph.advance_px);
          else if(found<0){free(blob);fufs_close(&archive);return 7;}
        }
      }else{
        printf("%u\t%08x\t%u\t%u\t%u\t%u\t%u\t%zu\n",
          i,archive.entries[i].id,font.codepoint_first,font.codepoint_last,
          font.glyph_count,font.line_height_px,font.total_advance_px,len);
      }
      count++;
    }
    free(blob);
  }
  fufs_close(&archive);
  return count?0:6;
}
