/* Independent host regression: source triangles, double-sided ray segments
 * and equivalent results from direct scanning vs XZ grid acceleration. */
#include "cnk_collision.h"
#include "cnk_geometry.h"
#include "cnk_scene.h"
#include "fufs.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int near(float a,float b){return fabsf(a-b)<0.002f;}
static void check(const cnk_geometry *g,const cnk_collision_grid *grid,
                  cnk_vec3 from,cnk_vec3 to){
 cnk_segment_hit direct={0},fast={0};
 int a=cnk_geometry_segment_cast(g,from,to,&direct);
 int b=cnk_collision_segment_cast(grid,from,to,&fast);
 assert(a==b);
 if(a==0){
  assert(near(direct.fraction,fast.fraction));
  assert(direct.triangle_index==fast.triangle_index);
  assert(direct.material==fast.material && direct.texture==fast.texture);
  assert(near(direct.point.x,fast.point.x));
  assert(near(direct.point.y,fast.point.y));
  assert(near(direct.point.z,fast.point.z));
  assert(near(direct.normal.x,fast.normal.x));
  assert(near(direct.normal.y,fast.normal.y));
  assert(near(direct.normal.z,fast.normal.z));
 }
}
static void synthetic(void){
 cnk_geo_triangle tris[3]={0};
 /* Horizontal floor and a vertical wall. */
 tris[0].v[0]=(cnk_geo_vertex){0,0,0,0,0,0};
 tris[0].v[1]=(cnk_geo_vertex){10,0,0,0,0,0};
 tris[0].v[2]=(cnk_geo_vertex){0,0,10,0,0,0};
 tris[0].material=8;tris[0].texture=14;
 tris[1].v[0]=(cnk_geo_vertex){5,-2,-5,0,0,0};
 tris[1].v[1]=(cnk_geo_vertex){5,2,-5,0,0,0};
 tris[1].v[2]=(cnk_geo_vertex){5,-2,5,0,0,0};
 cnk_geometry g={0};g.triangles=tris;g.triangle_count=2;
 cnk_collision_grid grid={0};
 assert(cnk_collision_grid_build(&g,8,8,&grid)==0);
 cnk_segment_hit hit={0};
 assert(cnk_collision_segment_cast(&grid,(cnk_vec3){1,4,1},(cnk_vec3){1,-4,1},&hit)==0);
 assert(near(hit.fraction,0.5f)&&hit.triangle_index==0);
 assert(hit.material==8 && hit.texture==14);
 assert(near(fabsf(hit.normal.y),1.0f));
 check(&g,&grid,(cnk_vec3){1,4,1},(cnk_vec3){1,-4,1});
 check(&g,&grid,(cnk_vec3){1,-4,1},(cnk_vec3){1,4,1});
 check(&g,&grid,(cnk_vec3){1,4,1},(cnk_vec3){1,3,1});
 check(&g,&grid,(cnk_vec3){-100,0,-100},(cnk_vec3){-99,1,-99});
 check(&g,&grid,(cnk_vec3){0,0,0},(cnk_vec3){9,0,9});
 check(&g,&grid,(cnk_vec3){4,0,-1},(cnk_vec3){6,0,-1});
 assert(cnk_collision_segment_cast(&grid,(cnk_vec3){1,1,1},(cnk_vec3){1,1,1},&hit)==-1);
 cnk_segment_hit preserve={.fraction=99.0f};
 assert(cnk_collision_segment_cast(&grid,(cnk_vec3){-40,2,-40},(cnk_vec3){-40,3,-40},&preserve)==-2);
 assert(preserve.fraction==99.0f);
 cnk_collision_grid_free(&grid);
}
int main(int argc,char **argv){
 assert(argc==2);synthetic();
 fufs_file vfs={0};assert(fufs_open(argv[1],&vfs)==0);
 unsigned scenes=0,queries=0,hits=0;
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *raw=NULL;size_t len=0;
  assert(fufs_extract(&vfs,i,&raw,&len)==0);
  int is_scene=len>=12&&!memcmp(raw,"PIFF",4)&&!memcmp(raw+8,"SCEN",4);
  if(!is_scene){free(raw);continue;}
  cnk_scene s={0};cnk_geometry g={0};cnk_collision_grid grid={0};
  assert(cnk_parse_scene(raw,len,&s)==0);free(raw);
  assert(cnk_geometry_build(&s,1.0f/64.0f,&g)==0);
  assert(cnk_collision_grid_build(&g,16,16,&grid)==0);
  /* Queries near original source route points: no fabricated track layout. */
  const cnk_sline *route=cnk_scene_line(&s,"DRV0");
  if(route && route->count){
   for(uint32_t k=0;k<route->count;k+=route->count/8+1){
    cnk_vec3 p=route->points[k];
    cnk_vec3 a={p.x,p.y+40,p.z},b={p.x,p.y-40,p.z};
    check(&g,&grid,a,b);queries++;
    cnk_segment_hit hit={0};
    if(!cnk_collision_segment_cast(&grid,a,b,&hit))hits++;
    /* Both-sided wall probes, including non-vertical surfaces. */
    a=(cnk_vec3){p.x-12,p.y+1,p.z-3};b=(cnk_vec3){p.x+12,p.y-1,p.z+3};
    check(&g,&grid,a,b);queries++;
   }
  }
  scenes++;
  cnk_collision_grid_free(&grid);cnk_geometry_free(&g);cnk_scene_free(&s);
 }
 fufs_close(&vfs);
 assert(scenes==14 && queries>=150 && hits>=45);
 printf("PASS segment cast: %u scenes, %u grid/direct comparisons, %u vertical hits; segment-wall/ground tests OK (NOT original Zeebo collision physics).\n",scenes,queries,hits);
 return 0;
}
