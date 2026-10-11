#include "cnk_render.h"
#include "cnk_camera.h"
#include "fufs.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int eq(float a,float b){return fabsf(a-b)<0.0001f;}
static int synthetic(void){
 cnk_geo_triangle t[4]={0};
 cnk_geometry source={0};source.triangles=t;source.triangle_count=4;
 const uint16_t materials[4]={9,2,9,2};
 for(int i=0;i<4;i++){
  t[i].material=materials[i];t[i].texture=1;
  for(int j=0;j<3;j++){
   t[i].v[j].x=(float)i;t[i].v[j].y=(float)j;t[i].v[j].z=1;
   t[i].v[j].u=(int16_t)(i*100+j);
   t[i].v[j].v=(int16_t)(i*50+j);
   t[i].v[j].uv_valid=1;
  }
 }
 cnk_render_mesh mesh={0};
 if(cnk_render_mesh_build(&source,&mesh))return 1;
 if(mesh.batch_count!=2||mesh.triangle_count!=4||mesh.vertex_count!=12||
    mesh.batches[0].material!=2||mesh.batches[0].vertex_count!=6||
    mesh.batches[1].material!=9||mesh.batches[1].first_vertex!=6)return 2;
 for(unsigned i=0;i<4;i++){
  unsigned original=mesh.source_triangles[i];
  const cnk_geo_triangle *src=&t[original];
  const cnk_render_vertex *v=&mesh.vertices[3*i];
  if(!eq(v[0].x,src->v[0].x)||!eq(v[1].y,src->v[1].y)||
     !eq(v[2].u,(float)src->v[2].u))return 3;
 }
 cnk_render_mesh_free(&mesh);
 cnk_camera cam={0};
 if(cnk_camera_build(&cam,(cnk_vec3){0,0,0},(cnk_vec3){0,0,10},
                     1.2f,16.0f/9.0f,0.1f,500.0f))return 4;
 float x,y;
 if(cnk_camera_project(&cam,(cnk_vec3){0,0,10},&x,&y) ||
    !eq(x,0)||!eq(y,0))return 5;
 /* Looking toward +Z, screen right is -X in a right-handed view. */
 if(cnk_camera_project(&cam,(cnk_vec3){1,0,10},&x,&y)||x>=0)return 6;
 if(cnk_camera_project(&cam,(cnk_vec3){0,0,-10},&x,&y)!=-2)return 7;
 cnk_kart kart={0};kart.position=(cnk_vec3){2,10,20};kart.heading=0;
 if(cnk_camera_follow(&cam,&kart,9,5,6,1.2f,16.0f/9.0f))return 8;
 if(!eq(cam.eye.x,2)||!eq(cam.eye.y,15)||!eq(cam.eye.z,11))return 9;
 if(cnk_camera_build(&cam,(cnk_vec3){0,0,0},(cnk_vec3){0,0,0},1,1,0.1f,100)!=-3)return 10;
 if(cnk_render_mesh_build(NULL,&mesh)==0)return 11;
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 int s=synthetic();if(s){fprintf(stderr,"Synthetic render/camera failed: %d\n",s);return 2;}
 fufs_file f={0};if(fufs_open(argv[1],&f))return 3;
 uint64_t total=0,batches=0,vertices=0;uint32_t scenes=0;
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t n=0;
  if(fufs_extract(&f,i,&data,&n)){fufs_close(&f);return 4;}
  if(n>=12 && !memcmp(data+8,"SCEN",4)){
   cnk_scene scene={0};cnk_geometry geo={0};cnk_render_mesh r={0};
   int rc=cnk_parse_scene(data,n,&scene);
   if(!rc)rc=cnk_geometry_build(&scene,1.0f/64.0f,&geo);
   if(!rc)rc=cnk_render_mesh_build(&geo,&r);
   if(rc){fprintf(stderr,"render scene %u error %d\n",i,rc);return 5;}
   if(!r.batch_count ||r.triangle_count!=geo.triangle_count ||
      r.vertex_count!=geo.triangle_count*3u)return 6;
   uint32_t count=0;
   for(uint32_t j=0;j<r.batch_count;j++){
    cnk_render_batch *b=&r.batches[j];
    if(b->first_vertex!=count || b->vertex_count%3 ||
       b->first_vertex+b->vertex_count>r.vertex_count)return 7;
    if(j && (b->material <r.batches[j-1].material ||
       (b->material==r.batches[j-1].material && b->texture<=r.batches[j-1].texture)))return 8;
    count+=b->vertex_count;
   }
   if(count!=r.vertex_count)return 9;
   uint8_t *seen=(uint8_t*)calloc(geo.triangle_count,1);
   if(!seen)return 10;
   for(uint32_t t=0;t<r.triangle_count;t++){
    uint32_t orig=r.source_triangles[t];
    if(orig>=geo.triangle_count || seen[orig]++)return 11;
    const cnk_geo_triangle *src=&geo.triangles[orig];
    for(int v=0;v<3;v++){
     cnk_render_vertex *rv=&r.vertices[t*3+(uint32_t)v];
     if(!eq(rv->x,src->v[v].x)||!eq(rv->y,src->v[v].y)||!eq(rv->z,src->v[v].z))return 12;
    }
   }
   free(seen);
   total+=r.triangle_count;batches+=r.batch_count;vertices+=r.vertex_count;scenes++;
   printf("scene %u: GPU triangles %u, draw batches %u\n",i,r.triangle_count,r.batch_count);
   cnk_render_mesh_free(&r);cnk_geometry_free(&geo);cnk_scene_free(&scene);
  }
  free(data);
 }
 fufs_close(&f);
 printf("RESULT: scenes=%u triangles=%llu vertices=%llu batches=%llu\n",
        scenes,(unsigned long long)total,(unsigned long long)vertices,(unsigned long long)batches);
 if(scenes!=14 || total!=102756 || vertices!=308268 || batches==0)return 13;
 puts("PASS: exact native scene vertex streams preserved in render batches; camera verified.");
 return 0;
}
