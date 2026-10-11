#ifndef CNK_COLLISION_H
#define CNK_COLLISION_H
#include "cnk_geometry.h"
#include <stdint.h>
/* Exact broad-phase for original scene triangles in XZ. This is a spatial
 * acceleration layer, not Polarbit's original kart collision model. */
typedef struct {
 const cnk_geometry *mesh; /* borrowed, must outlive grid */
 uint32_t cols,rows,count;
 float minx,minz,dx,dz;
 uint32_t *offsets; /* cols*rows+1; one exclusive range per cell */
 uint32_t *indices; /* triangle indices in packed cell ranges */
} cnk_collision_grid;
int cnk_collision_grid_build(const cnk_geometry *geometry, uint32_t cols, uint32_t rows,cnk_collision_grid *out);
void cnk_collision_grid_free(cnk_collision_grid *grid);
int cnk_collision_surface(const cnk_collision_grid *grid,float x,float z,float ref_y,
                          float *height,uint32_t *triangle_index);
/* Accelerated segment-against-original-triangles query. Identical hit
 * semantics to cnk_geometry_segment_cast; candidate search uses XZ grid.
 * Only tests the reconstructed subset (unplaced triangles are excluded).
 */
int cnk_collision_segment_cast(const cnk_collision_grid *grid,
 cnk_vec3 from,cnk_vec3 to,cnk_segment_hit *out);
/* Same source-triangle raycast, restricting the slope of returned faces.
 * max_abs_normal_y=0 admits vertical walls; =1 admits all surfaces.
 * No claim is made about which source polygons are actually collidable.
 * Miss/invalid arguments leave *out unchanged. */
int cnk_collision_segment_cast_steep(const cnk_collision_grid *grid,
 cnk_vec3 from,cnk_vec3 to,float max_abs_normal_y,cnk_segment_hit *out);
#endif
