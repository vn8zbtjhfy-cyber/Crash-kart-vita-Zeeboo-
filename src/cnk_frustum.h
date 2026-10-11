#ifndef CNK_FRUSTUM_H
#define CNK_FRUSTUM_H
#include "cnk_render.h"
#include "cnk_camera.h"
/* Conservative homogeneous clip-space AABB test. Does NOT infer missing
 * vertices or alter the original Zeebo mesh. Returns 1=possibly visible,
 * 0=definitely outside, -1=invalid input. Touching the frustum is visible. */
int cnk_batch_in_frustum(const cnk_camera *cam,const cnk_render_batch *batch);
#endif
