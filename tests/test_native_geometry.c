#include "cnk_geometry.h"
#include "fufs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

static int synthetic_test(void){
 cnk_svertex verts[3]={{0,0,0},{64,0,0},{0,64,64}};
 cnk_suv uvs[3]={{0,0},{1,0},{0,1}};
 uint16_t indices[3]={0,1,2};
 cnk_schunk chunk={0};chunk.index_count=3;chunk.indices=indices;
 cnk_voffset voff={0};voff.origin=(cnk_vec3){10,20,30};voff.start=0;voff.count=3;
 cnk_smaterial mat={0};char name[]="original.png";char *names[]={name};
 cnk_scene scene={0};scene.vertex_count=3;scene.uv_count=3;scene.voffset_count=1;
 scene.chunk_count=1;scene.material_count=1;scene.texture_count=1;
 scene.vertices=verts;scene.uvs=uvs;scene.chunks=&chunk;
 scene.voffsets=&voff;scene.materials=&mat;scene.texture_names=names;
 cnk_geometry geo;int rc=cnk_geometry_build(&scene,1.0f/64.0f,&geo);
 if(rc || geo.triangle_count!=1 || geo.unplaced_triangles ||
    fabsf(geo.triangles[0].v[1].x-11.0f)>1e-4f ||
    fabsf(geo.triangles[0].v[2].y-21.0f)>1e-4f){fprintf(stderr,"synthetic: conversion fail %d\n",rc);return 1;}
 float y=0;rc=cnk_geometry_nearest_surface(&geo,10.25f,30.25f,20.0f,&y,NULL);
 if(rc || fabsf(y-20.25f)>0.001f){fprintf(stderr,"synthetic: surface fail %d %f\n",rc,y);cnk_geometry_free(&geo);return 2;}
 if(cnk_geometry_nearest_surface(&geo,999,999,0,&y,NULL)!=-2){cnk_geometry_free(&geo);return 3;}
 cnk_geometry_free(&geo);
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"Usage: %s data.vfs\n",argv[0]);return 1;}
 if(synthetic_test())return 2;
 fufs_file f;if(fufs_open(argv[1],&f)){fprintf(stderr,"No ROM\n");return 3;}
 unsigned count=0;uint64_t total=0,converted=0,missing_uv=0,not_placed=0;
 uint32_t tested_points=0,found_surface=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t n=0;
  if(fufs_extract(&f,i,&data,&n)){fufs_close(&f);return 4;}
  if(n>=12 && !memcmp(data+8,"SCEN",4)){
   cnk_scene scene;cnk_geometry mesh;
   int rc=cnk_parse_scene(data,n,&scene);
   if(rc){fprintf(stderr,"scene %u err %d\n",i,rc);free(data);fufs_close(&f);return 5;}
   rc=cnk_geometry_build(&scene,1.0f/64.0f,&mesh);
   if(rc){fprintf(stderr,"geometry %u err %d\n",i,rc);cnk_scene_free(&scene);free(data);fufs_close(&f);return 6;}
   if(mesh.source_triangle_count!=mesh.triangle_count+mesh.unplaced_triangles ||
      !mesh.triangle_count || mesh.missing_uv_triangles>mesh.triangle_count){fprintf(stderr,"inconsistent geometry scene %u\n",i);return 7;}
   for(uint32_t j=0;j<scene.line_count;j++){
    if(strncmp(scene.lines[j].id,"DRV",3))continue;
    const cnk_sline *path=&scene.lines[j];
    for(uint32_t k=0;k<path->count;k+=path->count/10+1){
     cnk_vec3 loc=path->points[k];float surface;
     tested_points++;
     if(!cnk_geometry_nearest_surface(&mesh,loc.x,loc.z,loc.y,&surface,NULL)){
      found_surface++;
     }
    }
   }
   printf("SCEN %3u: tri=%5u placed=%5u unplaced=%4u uv-missing=%3u materials-no-name=%4u\n",
      i,mesh.source_triangle_count,mesh.triangle_count,mesh.unplaced_triangles,
      mesh.missing_uv_triangles,mesh.unnamed_texture_triangles);
   count++; total+=mesh.source_triangle_count;converted+=mesh.triangle_count;
   not_placed+=mesh.unplaced_triangles;missing_uv+=mesh.missing_uv_triangles;
   cnk_geometry_free(&mesh);cnk_scene_free(&scene);
  }
  free(data);
 }
 fufs_close(&f);
 printf("RESULT: scenes=%u source=%llu placed=%llu unknown-position=%llu missing-uv=%llu DRV-surface=%u/%u\n",
    count,(unsigned long long)total,(unsigned long long)converted,
    (unsigned long long)not_placed,(unsigned long long)missing_uv,found_surface,tested_points);
 if(count!=14 || total!=109452 || converted<80000 || found_surface<tested_points/4)return 8;
 puts("PASS: real geometry in native triangle buffers; remaining geometry explicitly reported, not fabricated.");
 return 0;
}
