/* Host audit: source-declared 3DOB POLY -> TEXL name, with NO PNG ID guesses. */
#include "fufs.h"
#include "cnk_model_material.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"Uso: %s data.vfs salida.csv\n",argv[0]);return 2;}
 fufs_file vfs={0};if(fufs_open(argv[1],&vfs)){fprintf(stderr,"No se pudo abrir ROM\n");return 3;}
 FILE *out=fopen(argv[2],"wb");if(!out){fufs_close(&vfs);return 4;}
 fputs("modelo_vfs_indice,modelo_id,grupo_poly,selector_material,nombre_texl_png,png_vfs_index,estado\n",out);
 unsigned valid=0,unresolved=0,named=0;
 for(unsigned idx=0;idx<vfs.count;idx++){
  unsigned char *blob=NULL;size_t n=0;
  if(fufs_extract(&vfs,idx,&blob,&n)){fclose(out);fufs_close(&vfs);return 5;}
  int isobj=n>=12&&!memcmp(blob,"PIFF",4)&&!memcmp(blob+8,"3DOB",4);
  free(blob);
  if(!isobj)continue;
  cnk_material_reference refs[512]={{0}};
  cnk_model model={0};cnk_model_texl texl={0};
  if(fufs_extract(&vfs,idx,&blob,&n))return 5;
  int ok=cnk_parse_3dob(blob,n,&model);
  if(!ok)ok=cnk_model_texl_parse(blob,n,&texl);
  free(blob);
  if(ok){fprintf(out,"%u,%08x,,,,,MODELO_NO_DECODIFICADO\n",idx,vfs.entries[idx].id);unresolved++;continue;}
  unsigned missing=0;
  ok=cnk_model_material_inspect(&model,&texl,refs,512,&missing);
  if(ok){cnk_model_free(&model);return 6;}
  if(missing)unresolved++;
  valid++;
  for(unsigned j=0;j<model.num_groups;j++){
   cnk_material_reference *r=&refs[j];
   const char *state=r->source_png[0]?"NOMBRE_TEXL_COMPROBADO_ID_PNG_DESCONOCIDO":
          r->original_selector?"SELECTOR_SIN_NOMBRE_TEXL":"SIN_TEXL";
   fprintf(out,"%u,%08x,%u,%u,%s,,%s\n",idx,vfs.entries[idx].id,j,
           r->original_selector,r->source_png,state);
   named+=!!r->source_png[0];
  }
  cnk_model_free(&model);
 }
 fclose(out);fufs_close(&vfs);
 printf("3DOB con referencias validas=%u; formatos especiales pendientes=%u; grupos con nombre=%u\n",valid,unresolved,named);
 return (valid==38 && unresolved==1 && named==50)?0:6;
}
