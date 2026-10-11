/* Reproducible byte-derived catalogue. No source strings are hardcoded here. */
#include "cnk_lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int escaped(const char *s){
 for(;*s;s++){
  if(*s=='\\'){if(fputs("\\\\",stdout)<0)return -1;}
  else if(*s=='\n'){if(fputs("\\n",stdout)<0)return -1;}
  else if(*s=='\r'){if(fputs("\\r",stdout)<0)return -1;}
  else if(*s=='\t'){if(fputs("\\t",stdout)<0)return -1;}
  else if(fputc((unsigned char)*s,stdout)==EOF)return -1;
 }
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"uso: %s data.vfs > catalogo_lang.tsv\n",argv[0]);return 2;}
 fufs_file source={0};if(fufs_open(argv[1],&source))return 3;
 if(puts("idioma_fuente\tindice_texto\ttexto_original_utf8")<0){fufs_close(&source);return 4;}
 int rc=0;
 for(unsigned locale=0;locale<CNK_LANG_LOCALES&&!rc;locale++){
  cnk_lang_pack pack={0};
  if(cnk_lang_load(&source,(cnk_lang_locale)locale,&pack)){rc=5;break;}
  for(unsigned j=0;j<pack.count;j++){
   if(escaped(pack.name)||printf("\t%u\t",j)<0||escaped(cnk_lang_text(&pack,j))||
      putchar('\n')==EOF){rc=6;break;}
  }
  cnk_lang_free(&pack);
 }
 if(ferror(stdout))rc=7;
 fufs_close(&source);return rc;
}
