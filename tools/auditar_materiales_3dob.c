/* Inventario reproducible de materiales POLY, tomado de la ROM Zeebo.
 * Los IDs identifican slots de material del modelo, NO PNG confirmados.
 * Uso: cnk_auditar_materiales data.vfs destino.csv
 */
#include "fufs.h"
#include "cnk_model.h"
#include "cnk_object_render.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int inventory(const char *vfs_path,const char *dest){
 fufs_file f={0};FILE *csv=NULL;
 uint32_t found=0,groups=0;uint64_t triangles=0;
 int status=1;
 if(fufs_open(vfs_path,&f)){fprintf(stderr,"No se pudo leer VFS: %s\n",vfs_path);return 2;}
 csv=fopen(dest,"w");
 if(!csv){fprintf(stderr,"No se pudo escribir %s: %s\n",dest,strerror(errno));goto done;}
 if(fputs("vfs_index,resource_id,source_vertices,source_triangles,group_index,material_id,index_type,group_triangles,"
          "bbox_xmin,bbox_ymin,bbox_zmin,bbox_xmax,bbox_ymax,bbox_zmax\n",csv)<0)goto done;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t size=0;
  if(fufs_extract(&f,i,&data,&size)!=0){fprintf(stderr,"Error en recurso %u\n",i);goto done;}
  if(size>=12 && !memcmp(data,"PIFF",4) && !memcmp(data+8,"3DOB",4)){
   cnk_model m={0};cnk_object_mesh mesh={0};
   if(cnk_parse_3dob(data,size,&m)!=0){free(data);fprintf(stderr,"Error al decodificar objeto %u\n",i);goto done;}
   if(cnk_object_mesh_build(&m,&mesh)!=0){cnk_model_free(&m);free(data);fprintf(stderr,"Error preparando objeto %u\n",i);goto done;}
   for(uint32_t j=0;j<mesh.batch_count;j++){
    const cnk_object_batch *b=&mesh.batches[j];
    if(fprintf(csv,"%u,%u,%u,%u,%u,%u,%u,%u,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g\n",
       i,f.entries[i].id,m.num_vertices,mesh.triangle_count,j,b->material,
       m.groups[j].original_gl_index_type,b->vertex_count/3,
       b->bbox_min[0],b->bbox_min[1],b->bbox_min[2],
       b->bbox_max[0],b->bbox_max[1],b->bbox_max[2])<0){
       cnk_object_mesh_free(&mesh);cnk_model_free(&m);free(data);goto done;
    }
   }
   groups+=mesh.batch_count;triangles+=mesh.triangle_count;found++;
   cnk_object_mesh_free(&mesh);cnk_model_free(&m);
  }
  free(data);
 }
 if(ferror(csv) || fflush(csv)!=0){fprintf(stderr,"Error al escribir CSV\n");goto done;}
 if(found!=38 || triangles!=8026){fprintf(stderr,"ROM diferente: objetos=%u triangulos=%llu\n",found,(unsigned long long)triangles);goto done;}
 printf("Objetos 3DOB: %u; grupos de material: %u; triangulos: %llu\n",found,groups,(unsigned long long)triangles);
 status=0;
 done:
 if(csv && fclose(csv)!=0)status=1;
 fufs_close(&f);
 if(status)remove(dest);
 return status;
}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"Uso: %s data.vfs inventario_materiales.csv\n",argv[0]);return 2;}
 return inventory(argv[1],argv[2]);
}
