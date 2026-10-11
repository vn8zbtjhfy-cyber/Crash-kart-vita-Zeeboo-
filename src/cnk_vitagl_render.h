#ifndef CNK_VITAGL_RENDER_H
#define CNK_VITAGL_RENDER_H
#include "cnk_render.h"
#include "cnk_camera.h"
/* Native Vita GPU backend. Requires vitaGL + VitaSDK.
 * Not a full-game renderer: animations, original texture ID mapping,
 * lighting, transparency, track objects and original camera not recovered. */
/* Returns OpenGL texture handle from original SCEN texture ID, or 0 if not
 * decoded. Never generates a random/replacement texture. */
typedef unsigned int (*cnk_native_texture_resolve)(void *ctx, uint16_t texture);
int cnk_vitagl_draw_scene(const cnk_render_mesh *mesh,const cnk_camera *camera,
                           cnk_native_texture_resolve resolve,void *context,
                           unsigned *draw_calls);
/* Optional conservatively culled track renderer. Uses the original AABBs;
 * no material or geometry is invented. Returning 0 still means host source
 * validation, NOT actual Vita GPU performance or visual equivalence. */
int cnk_vitagl_draw_scene_culled(const cnk_render_mesh *mesh,const cnk_camera *camera,
   cnk_native_texture_resolve resolve,void *context,
   unsigned *draw_calls,uint32_t *visible_triangles);
#endif
