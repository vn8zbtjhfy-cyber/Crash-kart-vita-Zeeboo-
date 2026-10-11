#ifndef CNK_VITAGL_ITEMS_H
#define CNK_VITAGL_ITEMS_H
#include "cnk_item_visual.h"
#include "cnk_camera.h"
/* Callback returns zero if a reviewed, original PNG association is unavailable.
 * Unmapped actors use a color-coded DEVELOPMENT marker, never fake artwork. */
typedef unsigned (*cnk_item_sprite_resolve)(void *,cnk_item_sprite);
int cnk_vitagl_draw_items(const cnk_item_visual_frame *actors,const cnk_camera *camera,
                         cnk_item_sprite_resolve resolve,void *context,
                         unsigned *draw_calls);
#endif
