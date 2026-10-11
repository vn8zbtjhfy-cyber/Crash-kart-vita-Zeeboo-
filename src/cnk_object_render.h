#ifndef CNK_OBJECT_RENDER_H
#define CNK_OBJECT_RENDER_H
#include "cnk_model.h"
#include "cnk_render.h"
#include "fufs.h"
#include <stdint.h>
/* Prepared 3DOB draw data. The vertices, triangle order and source material
 * IDs are exact from the ROM. Material->texture and world placement are NOT
 * inferred here: both need explicit verification against the original game. */
typedef struct {
  uint32_t material;
  uint32_t first_vertex,vertex_count;
  float bbox_min[3],bbox_max[3];
} cnk_object_batch;
typedef struct {
  cnk_render_vertex *vertices;
  uint16_t *source_indices; /* source MESH vertex for each expanded vertex */
  cnk_object_batch *batches;
  uint32_t vertex_count,triangle_count,batch_count;
  float bbox_min[3],bbox_max[3];
} cnk_object_mesh;
/* Error codes < 0. Output cleared on failure; free with mesh_free. */
int cnk_object_mesh_build(const cnk_model *src,cnk_object_mesh *out);
void cnk_object_mesh_free(cnk_object_mesh *out);
/* Decode an exact caller-supplied 3DOB VFS slot into GPU-ready triangles.
 * No guesses about character/cart identity or placement. Caller owns result. */
int cnk_object_mesh_from_vfs(const fufs_file *vfs,uint32_t source_index,
                             cnk_object_mesh *out);
#endif
