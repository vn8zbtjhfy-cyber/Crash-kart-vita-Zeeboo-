#ifndef CNK_RENDER_H
#define CNK_RENDER_H
#include "cnk_geometry.h"
#include <stdint.h>
/* Prepares original Zeebo SCEN mesh for a native GPU draw backend.
 * World positions and the ORIGINAL raw UV coordinates are preserved.
 * No invented colours or guessed texture scale are stored in this format.
 * The actual texture lookup/UV normalization needs original-game validation. */
typedef struct { float x,y,z,u,v; } cnk_render_vertex;
typedef struct {
 uint16_t material,texture;
 uint32_t first_vertex,vertex_count;
 float bbox_min[3],bbox_max[3];
} cnk_render_batch;
typedef struct {
 cnk_render_vertex *vertices;
 uint32_t *source_triangles; /* source index per 3 vertices */
 cnk_render_batch *batches;
 uint32_t vertex_count,triangle_count,batch_count;
 uint32_t skipped_invalid_uv; /* no bogus guessed coordinates */
} cnk_render_mesh;
int cnk_render_mesh_build(const cnk_geometry *src,cnk_render_mesh *out);
void cnk_render_mesh_free(cnk_render_mesh *mesh);
#endif
