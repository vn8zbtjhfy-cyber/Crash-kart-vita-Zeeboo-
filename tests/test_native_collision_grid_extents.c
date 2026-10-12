/* Synthetic geometry only: malformed scene bounds must never create a
 * spatial grid with INF/NaN cell widths or undefined float->index casts. */
#include "cnk_collision.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static cnk_geo_triangle triangle(float x0,float z0,float x1,float z1,float x2,float z2){
 cnk_geo_triangle t={0};
 t.v[0].x=x0;t.v[0].z=z0;
 t.v[1].x=x1;t.v[1].z=z1;
 t.v[2].x=x2;t.v[2].z=z2;
 return t;
}
static void reject_extreme(cnk_geo_triangle *tris){
 cnk_geometry mesh={0};mesh.triangles=tris;mesh.triangle_count=2;
 cnk_collision_grid grid={0};
 assert(cnk_collision_grid_build(&mesh,32,32,&grid)<0);
 assert(grid.mesh==NULL && grid.offsets==NULL && grid.indices==NULL);
}
int main(void){
 cnk_geo_triangle tris[2]={
  triangle(-FLT_MAX,0,0,0,-FLT_MAX,1),
  triangle( FLT_MAX,0,0,0, FLT_MAX,1)
 };
 /* All vertices are finite, but the X extent overflows float. */
 reject_extreme(tris);
 tris[0]=triangle(0,-FLT_MAX,1,-FLT_MAX,0,0);
 tris[1]=triangle(0, FLT_MAX,1, FLT_MAX,0,0);
 /* The Z extent must receive exactly the same protection. */
 reject_extreme(tris);
 /* Legitimate, bounded triangles still build and answer ground queries. */
 tris[0]=triangle(0,0,2,0,0,2);
 cnk_geometry mesh={0};mesh.triangles=tris;mesh.triangle_count=1;
 cnk_collision_grid grid={0};
 assert(cnk_collision_grid_build(&mesh,8,8,&grid)==0);
 assert(isfinite(grid.dx)&&isfinite(grid.dz)&&grid.dx>0&&grid.dz>0);
 float ground=-1.0f;
 assert(cnk_collision_surface(&grid,0.5f,0.5f,0,&ground,NULL)==0);
 assert(fabsf(ground)<0.00001f);
 cnk_collision_grid_free(&grid);
 puts("GRID EXTENTS PASS: overflow bounds rejected, ordinary source-style mesh retained");
 return 0;
}
