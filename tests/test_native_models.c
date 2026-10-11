/* Compilación: cc -std=c99 -Wall -Wextra -O2 -Isrc tests/test_native_models.c
 *              src/fufs.c src/cnk_model.c -lz -o test_native_models
 *              && ./test_native_models data.vfs
 * Verifica TODO el grupo PIFF sin interpretar ni emular el código BREW. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fufs.h"
#include "cnk_model.h"
int main(int argc,char **argv){
 fufs_file f;uint32_t i,models=0,scenes=0;uint64_t vertices=0,triangles=0;
 if(argc!=2){fprintf(stderr,"Uso: %s data.vfs\n",argv[0]);return 2;}
 if(fufs_open(argv[1],&f)){fprintf(stderr,"Error abriendo VFS\n");return 3;}
 for(i=0;i<f.count;i++){
  unsigned char *p=NULL;size_t n=0;
  if(fufs_extract(&f,i,&p,&n)){fprintf(stderr,"Error recurso %u\n",i);return 4;}
  if(n>=12 && !memcmp(p,"PIFF",4)){
   if(!memcmp(p+8,"3DOB",4)){
    cnk_model m;
    if(cnk_parse_3dob(p,n,&m)){
      fprintf(stderr,"Error decodificando geometria del recurso %u\n",i);free(p);return 5;
    }
    models++;vertices+=m.num_vertices;
    for(uint32_t j=0;j<m.num_groups;j++)triangles+=m.groups[j].count/3;
    cnk_model_free(&m);
   }else if(!memcmp(p+8,"SCEN",4)) scenes++;
   else{fprintf(stderr,"Tipo PIFF inesperado: %u\n",i);free(p);return 6;}
  }
  free(p);
 }
 fufs_close(&f);
 printf("3DOB=%u SCEN=%u vertices=%llu triangulos=%llu\n",models,scenes,
 (unsigned long long)vertices,(unsigned long long)triangles);
 if(models!=38 || scenes!=14 || vertices!=10564 || triangles!=8026){
  fprintf(stderr,"Fallo: inventario geometrico distinto de la ROM base\n");return 7;
 }
 puts("PASADO: modelos 3DOB decodificados de la ROM original sin indices invalidos");
 return 0;
}
