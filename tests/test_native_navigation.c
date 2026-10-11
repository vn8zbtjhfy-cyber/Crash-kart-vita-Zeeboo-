#include "cnk_scene.h"
#include "cnk_nav.h"
#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static float displacement(cnk_vec3 a,cnk_vec3 b){
 float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return sqrtf(x*x+y*y+z*z);
}
int main(int argc,char **argv){
 if(argc!=2)return 2;
 fufs_file f;if(fufs_open(argv[1],&f))return 3;
 int tested=0,fail=0;double lengths=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t len=0;if(fufs_extract(&f,i,&data,&len)){fail=4;break;}
  if(len>=12 && !memcmp(data,"PIFF",4) && !memcmp(data+8,"SCEN",4)){
   cnk_scene scene;if(cnk_parse_scene(data,len,&scene)){free(data);fail=5;break;}
   for(uint32_t j=0;j<scene.line_count;j++){
    const cnk_sline *l=&scene.lines[j];
    if(strncmp(l->id,"DRV",3))continue;
    cnk_vec3 start,end,mid;
    float length=cnk_nav_length(l,0);
    if(!isfinite(length) || length<=0 ||
       cnk_nav_sample(l,0,0,&start) ||
       cnk_nav_sample(l,1,0,&end) ||
       cnk_nav_sample(l,0.5f,0,&mid) ||
       displacement(start,l->points[0])>0.0001f ||
       displacement(end,l->points[l->count-1])>0.01f){fprintf(stderr,"Error en recurso %u ruta %s inicio=%.5f fin=%.5f longitud=%f\n",i,l->id,displacement(start,l->points[0]),displacement(end,l->points[l->count-1]),length);fail=6;break;}
    cnk_nav_projection projected;
    if(cnk_nav_nearest(l,mid,0,&projected) || projected.distance>0.05f ||
       fabsf(projected.fraction-0.5f)>0.01f){fail=7;break;}
    tested++;lengths+=length;
   }
   cnk_scene_free(&scene);
  }
  free(data);
  if(fail)break;
 }
 fufs_close(&f);
 printf("NAVEGACION: rutas=%d suma_longitudes=%.1f\n",tested,lengths);
 if(fail){fprintf(stderr,"FALLO: %d\n",fail);return fail;}
 if(tested!=37)return 8;
 /* Verificación algebraica de cruces; NO es una pista adicional. */
 cnk_vec3 gate_points[2]={{0,0,-1},{0,0,1}};
 cnk_sline gate={{'L','A','P','0',0},2,gate_points};
 cnk_vec3 left={-1,0,0},right={1,0,0};
 cnk_vec3 off_left={-1,0,10},off_right={1,0,10};
 if(cnk_nav_gate_crossing(&gate,left,right)!=-1 ||
    cnk_nav_gate_crossing(&gate,right,left)!=1 ||
    cnk_nav_gate_crossing(&gate,off_left,off_right)!=0) return 9;
 puts("PASADO: consulta de ruta, interpolacion y proyeccion sobre puntos ORIGINALES.");
 return 0;
}
