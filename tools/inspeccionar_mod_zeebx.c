/* User-owned CNK2 executable information, based on the structural
 * parsers adapted from Zeebx. Never executes BREW code and never writes
 * or uploads either game file. GPL-2.0-or-later applies to parser. */
#include "cnk_zeebx_brew.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static int read_file(const char *path,unsigned char **buf,size_t *length){
 FILE *f=fopen(path,"rb");
 if(!f)return -1;
 if(fseek(f,0,SEEK_END)){fclose(f);return -2;}
 long n=ftell(f);
 if(n<=0||n>32*1024*1024L||fseek(f,0,SEEK_SET)){
  fclose(f);return -3;
 }
 unsigned char *bytes=(unsigned char*)malloc((size_t)n);
 if(!bytes){fclose(f);return -4;}
 size_t read=fread(bytes,1,(size_t)n,f);
 fclose(f);
 if(read!=(size_t)n){free(bytes);return -5;}
 *buf=bytes;*length=(size_t)n;
 return 0;
}
int main(int argc,char **argv){
 if(argc!=3){
  fprintf(stderr,"uso: %s /ruta/cnk2.mod /ruta/274214.mif\n",argv[0]);
  return 2;
 }
 unsigned char *module=NULL,*manifest=NULL;
 size_t mbytes=0,nbytes=0;
 cnk_brew_mod_info mod={0};
 cnk_brew_mif_info mif={0};
 int rc=1;
 if(read_file(argv[1],&module,&mbytes)||read_file(argv[2],&manifest,&nbytes)){
  fprintf(stderr,"No pude abrir ambos archivos originales locales.\n");
  goto done;
 }
 if(cnk_brew_mod_inspect(module,mbytes,&mod)||
    cnk_brew_mif_inspect(manifest,nbytes,&mif)){
  fprintf(stderr,"Módulo BREW o manifiesto MIF inválido.\n");
  goto done;
 }
 printf("MOD %lu bytes, variante %s, entrada virtual ARM 0x%08x\n",
        (unsigned long)mbytes,mod.has_brew_header?"BREW HEADER":"RAW",
        mod.entry_offset);
 if(mod.has_initial_branch)
  printf("Primer branch ARM desde 0 -> 0x%08x\n",mod.first_branch_target);
 printf("MIF %lu bytes, secciones %u, applet ClassID principal 0x%08x\n",
        (unsigned long)nbytes,mif.sections,mif.main_applet);
 for(uint32_t i=0;i<mif.class_count;i++)
  printf("Clase BREW registrada: 0x%08x\n",mif.classes[i]);
 printf("Diagnóstico de formato solamente; el port nativo no ejecuta el MOD.\n");
 rc=0;
done:
 free(module);free(manifest);
 return rc;
}
