#ifndef CNK_VITAGL_OBJECTS_H
#define CNK_VITAGL_OBJECTS_H
#include "cnk_object_render.h"
#include "cnk_camera.h"
#include <stdint.h>
/* The caller supplies a verified source-model world transform and an explicitly
 * validated material->GL texture mapping. A missing mapping must return 0;
 * this code never guesses character identities, skins or placements. */
typedef unsigned int (*cnk_object_material_texture)(void *ctx,uint32_t material);
int cnk_vitagl_draw_object(const cnk_object_mesh *mesh,
                          const cnk_camera *camera,
                          const float world_from_model[16],
                          cnk_object_material_texture resolve,void *ctx,
                          unsigned *draw_calls);
/* Opt-in material-batch frustum culling using original 3DOB bounding boxes.
 * No calls, no GL state mutation on malformed geometry or camera. */
int cnk_vitagl_draw_object_culled(const cnk_object_mesh *mesh,
                          const cnk_camera *camera,
                          const float world_from_model[16],
                          cnk_object_material_texture resolve,void *ctx,
                          unsigned *draw_calls,uint32_t *visible_triangles);
#endif
