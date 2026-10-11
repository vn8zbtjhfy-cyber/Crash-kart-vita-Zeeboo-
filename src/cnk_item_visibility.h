#ifndef CNK_ITEM_VISIBILITY_H
#define CNK_ITEM_VISIBILITY_H
#include "cnk_item_visual.h"
#include "cnk_camera.h"
/* Filters billboards whose entire sphere of influence is outside the
 * actual camera clip volume, and stably orders remaining translucent sprites
 * far-to-near. Geometry/data aren't modified. A conservative sphere ensures
 * we never discard a visible corner of a camera-facing quad.
 * Invalid camera / actor => output and counters remain unchanged. */
int cnk_item_visibility_prepare(const cnk_item_visual_frame *input,
    const cnk_camera *camera,cnk_item_visual_frame *output,
    unsigned *culled);
#endif
