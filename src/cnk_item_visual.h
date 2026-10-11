#ifndef CNK_ITEM_VISUAL_H
#define CNK_ITEM_VISUAL_H
#include "cnk_items.h"
#include <stddef.h>
/* Source-independent, read-only snapshot of item actors for the Vita renderer.
 * A visual is NOT evidence of historical Zeebo artwork or sprite dimensions. */
typedef enum {
 CNK_ITEM_SPRITE_WUMPA=0, CNK_ITEM_SPRITE_CRATE=1,
 CNK_ITEM_SPRITE_BOMB=2, CNK_ITEM_SPRITE_MISSILE=3,
 CNK_ITEM_SPRITE_TNT=4, CNK_ITEM_SPRITE_OIL=5,
 CNK_ITEM_SPRITE_COUNT=6
} cnk_item_sprite;
#define CNK_ITEM_VISUAL_CAPACITY (CNK_ITEM_MAX_PICKUPS+CNK_ITEM_MAX_BOMBS+CNK_ITEM_MAX_MISSILES+CNK_ITEM_MAX_HAZARDS)
typedef struct {
 cnk_vec3 position;
 float half_size; /* provisional development-only size in world units */
 cnk_item_sprite sprite;
} cnk_item_visual;
typedef struct {
 cnk_item_visual actors[CNK_ITEM_VISUAL_CAPACITY];
 size_t count;
} cnk_item_visual_frame;
/* No mutation of *out on invalid active actor. Unavailable crates are hidden;
 * inactive projectile/trap pool slots are always hidden. */
int cnk_item_visual_collect(const cnk_item_world *world,cnk_item_visual_frame *out);
#endif
