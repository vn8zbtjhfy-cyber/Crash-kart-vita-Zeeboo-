#include "cnk_item_visibility.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
int main(void){
 cnk_camera cam={0};
 assert(!cnk_camera_build(&cam,(cnk_vec3){0,2,-10},
    (cnk_vec3){0,0,0},1.1f,960.f/544.f,.1f,100.f));
 cnk_item_visual_frame input={0},result={0};unsigned culled=77;
 input.count=5;
 input.actors[0]=(cnk_item_visual){{0,0,1},.7f,CNK_ITEM_SPRITE_WUMPA};
 input.actors[1]=(cnk_item_visual){{0,0,20},.7f,CNK_ITEM_SPRITE_BOMB};
 input.actors[2]=(cnk_item_visual){{0,0,4},.7f,CNK_ITEM_SPRITE_TNT};
 input.actors[3]=(cnk_item_visual){{0,0,-40},.7f,CNK_ITEM_SPRITE_MISSILE};
 input.actors[4]=(cnk_item_visual){{200,0,0},.7f,CNK_ITEM_SPRITE_OIL};
 assert(!cnk_item_visibility_prepare(&input,&cam,&result,&culled));
 assert(result.count==3&&culled==2);
 assert(result.actors[0].sprite==CNK_ITEM_SPRITE_BOMB);
 assert(result.actors[1].sprite==CNK_ITEM_SPRITE_TNT);
 assert(result.actors[2].sprite==CNK_ITEM_SPRITE_WUMPA);
 /* Idempotent ordering and in-place filtering. */
 assert(!cnk_item_visibility_prepare(&result,&cam,&result,&culled));
 assert(result.count==3&&culled==0);
 cnk_item_visual_frame before=result;
 input.actors[0].position.y=NAN;
 assert(cnk_item_visibility_prepare(&input,&cam,&result,&culled)<0);
 assert(memcmp(&before,&result,sizeof(result))==0&&culled==0);
 input.actors[0].position.y=0;
 cam.view_projection[5]=NAN;
 assert(cnk_item_visibility_prepare(&input,&cam,&result,&culled)<0);
 assert(memcmp(&before,&result,sizeof(result))==0);
 assert(!cnk_camera_build(&cam,(cnk_vec3){0,2,-10},
    (cnk_vec3){0,0,0},1.1f,960.f/544.f,.1f,100.f));
 /* Boundary actor with large billboard may be partially visible: keep it. */
 input.count=1;input.actors[0]=(cnk_item_visual){{8.0f,0,0},4.f,CNK_ITEM_SPRITE_CRATE};
 assert(!cnk_item_visibility_prepare(&input,&cam,&result,&culled));
 assert(result.count==1);
 /* Two ties preserve their source identity to avoid sprite flicker. */
 input.count=2;
 input.actors[0]=(cnk_item_visual){{0,0,2},1.f,CNK_ITEM_SPRITE_BOMB};
 input.actors[1]=(cnk_item_visual){{0,0,2},1.f,CNK_ITEM_SPRITE_OIL};
 assert(!cnk_item_visibility_prepare(&input,&cam,&result,&culled));
 assert(result.count==2&&result.actors[0].sprite==CNK_ITEM_SPRITE_BOMB&&
        result.actors[1].sprite==CNK_ITEM_SPRITE_OIL);
 puts("PASS: source-accurate sprite frustum filtering and stable transparency depth order");
 return 0;
}
