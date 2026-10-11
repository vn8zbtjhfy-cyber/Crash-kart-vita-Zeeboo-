#ifndef CNK_GEOMETRY_H
#define CNK_GEOMETRY_H
#include "cnk_scene.h"
/* Conversion of *verified* SCEN static geometry into a portable triangle
 * stream suitable for a native rendering/collision backend. Coordinates in
 * SCEN VERT are local signed integers. The /64 position scale is a tested
 * working hypothesis, NOT a demonstrated match to the original renderer.
 * Other unassigned vertices are not silently given guessed positions. */

typedef struct {
  float x,y,z;
  int16_t u,v; /* original UV values, normalisation unknown */
  uint8_t uv_valid;
} cnk_geo_vertex;

typedef struct {
  cnk_geo_vertex v[3];
  uint16_t material;
  uint16_t texture;
} cnk_geo_triangle;

typedef struct {
  cnk_geo_triangle *triangles;
  uint32_t triangle_count;
  uint32_t source_triangle_count;
  uint32_t unplaced_triangles;
  uint32_t missing_uv_triangles;
  uint32_t unnamed_texture_triangles;
  float scale;
} cnk_geometry;

/* `scale`=1/64 is likely for most tile-local scene vertices, awaiting game
 * comparison. Caller owns geometry and must call cnk_geometry_free. */
int cnk_geometry_build(const cnk_scene *scene, float scale, cnk_geometry *out);
void cnk_geometry_free(cnk_geometry *g);
/* The *first* intersection of a bounded three-dimensional segment with
 * source scene triangles. Both sides of a face count; no texture/material
 * is assumed to be drivable. This supports future camera obstruction,
 * walls and effects, but does NOT reproduce Zeebo's physics response.
 * A coplanar segment is deliberately not treated as a point hit.
 * Return 0 for a hit, -2 for no hit, -1 for invalid input.
 * On a miss/error, output stays unchanged.
 */
typedef struct {
 float fraction; /* [0,1] from 'from' to 'to' */
 cnk_vec3 point;
 cnk_vec3 normal; /* normal from original triangle winding */
 uint32_t triangle_index;
 uint16_t material,texture;
} cnk_segment_hit;
/* Common intersection predicate used by both direct and grid queries. */
int cnk_geometry_triangle_segment(const cnk_geo_triangle *triangle,
 cnk_vec3 from,cnk_vec3 to,float *fraction,cnk_vec3 *normal);
int cnk_geometry_segment_cast(const cnk_geometry *mesh,
 cnk_vec3 from,cnk_vec3 to,cnk_segment_hit *out);

/* Find height of a surface under x,z closest to reference_y. This query is
 * geometric only: NOT an original-game collision response or tyre physics. */
int cnk_geometry_nearest_surface(const cnk_geometry *g,
                                 float x,float z,float reference_y,
                                 float *surface_y,uint32_t *triangle_index);
#endif
