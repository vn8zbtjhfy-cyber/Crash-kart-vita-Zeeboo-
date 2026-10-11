#include "cnk_geometry.h"
#include "cnk_collision.h"
#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"Uso: %s data.vfs\n",argv[0]);return 2;}
 fufs_file f;if(fufs_open(argv[1],&f))return 3;
 unsigned scene_count=0,passes=0,misses=0;uint64_t refs=0;
 for(uint32_t i=0;i<f.count;i++){
   unsigned char *data=NULL;size_t n=0;
   if(fufs_extract(&f,i,&data,&n)){fufs_close(&f);return 4;}
   if(n>=12 && !memcmp(data+8,"SCEN",4)){
     cnk_scene scene;cnk_geometry mesh;cnk_collision_grid grid;
     int status=cnk_parse_scene(data,n,&scene);
     if(status){fprintf(stderr,"SCEN %u parse err %d\n",i,status);return 5;}
     status=cnk_geometry_build(&scene,1.0f/64.0f,&mesh);
     if(status){fprintf(stderr,"SCEN %u geom err %d\n",i,status);return 6;}
     status=cnk_collision_grid_build(&mesh,32,32,&grid);
     if(status){fprintf(stderr,"SCEN %u grid err %d\n",i,status);return 7;}
     refs+=grid.count;
     for(uint32_t j=0;j<scene.line_count;j++){
       const cnk_sline *line=&scene.lines[j];
       if(strncmp(line->id,"DRV",3))continue;
       for(uint32_t k=0;k<line->count;k+=11){
         cnk_vec3 p=line->points[k];float direct=0,fast=0;
         int a=cnk_geometry_nearest_surface(&mesh,p.x,p.z,p.y,&direct,NULL);
         int b=cnk_collision_surface(&grid,p.x,p.z,p.y,&fast,NULL);
         if((!a)!=(!b) || (!a&&fabsf(fast-direct)>0.001f)){
           fprintf(stderr,"SCEN %u DRV %s #%u mismatch direct=%d fast=%d %.6f %.6f\n",
                   i,line->id,k,a,b,direct,fast);
           return 8;
         }
         if(!a)passes++;else misses++;
       }
     }
     scene_count++;
     printf("scene %-3u grid=32x32 references=%u triangles=%u\n",i,grid.count,mesh.triangle_count);
     cnk_collision_grid_free(&grid);cnk_geometry_free(&mesh);cnk_scene_free(&scene);
   }
   free(data);
 }
 fufs_close(&f);
 printf("COMPARISON PASSED: %u scenes, %u matching hits, %u matching empty queries, %llu grid refs\n",
        scene_count,passes,misses,(unsigned long long)refs);
 return scene_count==14 && passes>400 && misses<=10 ? 0:9;
}
