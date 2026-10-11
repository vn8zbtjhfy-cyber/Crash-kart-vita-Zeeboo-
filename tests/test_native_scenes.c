/* Prueba exhaustiva sobre la ROM entregada: los datos de circuito y rutas
 * NO son contenido de muestra. cc ... -lz ... data.vfs */
#include "cnk_scene.h"
#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
int main(int argc,char **argv){
 fufs_file f;uint32_t tracks=0,paths=0,chunks=0,lap=0;uint64_t verts=0,triangles=0,waypoints=0;
 if(argc!=2){fprintf(stderr,"Uso: %s data.vfs\n",argv[0]);return 2;}
 if(fufs_open(argv[1],&f)){fputs("No se puede abrir data.vfs\n",stderr);return 3;}
 int failure=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *p=NULL;size_t n=0;
  if(fufs_extract(&f,i,&p,&n)){failure=4;break;}
  if(n>=12 && !memcmp(p,"PIFF",4) && !memcmp(p+8,"SCEN",4)){
   cnk_scene scene;
   int status=cnk_parse_scene(p,n,&scene);
   if(status){fprintf(stderr,"SCEN %u: error %d\n",i,status);free(p);failure=5;break;}
   tracks++;verts+=scene.vertex_count;chunks+=scene.chunk_count;
   for(uint32_t j=0;j<scene.chunk_count;j++)triangles+=scene.chunks[j].index_count/3;
   for(uint32_t j=0;j<scene.line_count;j++){
    const cnk_sline *line=&scene.lines[j];
    if(!strncmp(line->id,"DRV",3)){paths++;waypoints+=line->count;}
    if(!strncmp(line->id,"LAP",3))lap++;
   }
   printf("SCEN #%3u: vertices=%5u grupos=%4u rutas=%2u posiciones=%u\n",i,
          scene.vertex_count,scene.chunk_count,scene.line_count,scene.position_count);
   /* Rejección de materiales corruptos y referencias al índice incorrectas. */
   for(uint32_t j=0;j<scene.chunk_count;j++){
    cnk_schunk *c=&scene.chunks[j];
    if(!c->indices || c->material>=scene.material_count){failure=6;break;}
   }
   cnk_scene_free(&scene);
  }
  free(p);
  if(failure)break;
 }
 fufs_close(&f);
 printf("RESULTADO: escenas=%u vertices=%llu grupos=%u triangulos=%llu rutasDRV=%u puntosDRV=%llu lineasLAP=%u\n",
        tracks,(unsigned long long)verts,chunks,(unsigned long long)triangles,
        paths,(unsigned long long)waypoints,lap);
 if(failure)return failure;
 if(tracks!=14 || verts!=163553 || paths<30 || lap<30 || chunks<5000)return 7;
 puts("PASADO: todas las pistas originales, triangulos, materiales e indices de sus rutas validos.");
 return 0;
}
