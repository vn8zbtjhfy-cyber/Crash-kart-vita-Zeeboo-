#include "cnk_vitagl_items.h"
#include "cnk_vitagl_game_frame.h"
#include "vitaGL.h"
#include "cnk_track_catalog.h"
#include "cnk_vitagl_textures.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
static unsigned known(void *c,cnk_item_sprite s){
 unsigned *n=c;(*n)++;return s==CNK_ITEM_SPRITE_MISSILE?111u:0u;
}
static unsigned uploaded(void *ctx,cnk_item_sprite sprite){
 return cnk_vitagl_textures_resolve(ctx,(uint16_t)sprite);
}
int main(int argc,char **argv){
 assert(argc==2);
 cnk_camera cam={0};
 assert(!cnk_camera_build(&cam,(cnk_vec3){0,4,-10},(cnk_vec3){0,0,0},
                          1.1f,960.0f/544.0f,0.1f,100.0f));
 cnk_item_visual_frame frame={0};unsigned calls=17,lookups=0;
 cnk_test_gl_reset(100);
 assert(!cnk_vitagl_draw_items(&frame,&cam,known,&lookups,&calls));
 assert(calls==0&&cnk_test_gl.draw_calls==0&&lookups==0);
 frame.count=3;
 frame.actors[0]=(cnk_item_visual){{1,0,2},1,CNK_ITEM_SPRITE_WUMPA};
 frame.actors[1]=(cnk_item_visual){{0,0,3},0.6f,CNK_ITEM_SPRITE_MISSILE};
 frame.actors[2]=(cnk_item_visual){{-2,0,3},0.5f,CNK_ITEM_SPRITE_TNT};
 assert(!cnk_vitagl_draw_items(&frame,&cam,known,&lookups,&calls));
 assert(calls==3&&lookups==3&&cnk_test_gl.draw_calls==3);
 assert(cnk_test_gl.drawn_vertices==18);
 assert(cnk_test_gl.textured_calls==1&&cnk_test_gl.untextured_calls==2);
 assert(cnk_test_gl.invalid==0);
 cnk_test_gl_reset(100);calls=13;
 frame.actors[2].position.y=NAN;
 assert(cnk_vitagl_draw_items(&frame,&cam,known,&lookups,&calls)<0);
 assert(cnk_test_gl.draw_calls==0&&calls==13);
 frame.actors[2].position.y=0;
 fufs_file vfs={0};cnk_track_catalog tracks={0};cnk_game game={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(!cnk_track_catalog_build(&vfs,&tracks));
 const cnk_track_entry *source=cnk_track_race_at(&tracks,0);assert(source);
 assert(!cnk_game_load(&game,&vfs,source->vfs_index,3,2));
 /* Deterministic test camera keeps synthetic actors inside the viewport. */
 game.session.camera=cam;
 game.session.items.bombs[0]=(cnk_item_bomb){{1,0,1},0,2,0,1};
 game.session.items.missiles[0]=(cnk_item_missile){{1,0,3},0,2,0,UINT32_MAX,1};
 game.session.items.hazards[0]=(cnk_item_hazard){{3,0,3},2,0,CNK_WEAPON_OIL,1};
 cnk_test_gl_reset(game.render.vertex_count);
 cnk_vitagl_frame_result result={0};
 assert(!cnk_vitagl_draw_game_frame_with_items_culled(&game,NULL,NULL,NULL,NULL,NULL,NULL,&result));
 assert(result.item_actors==3&&result.item_draw_calls==3&&result.item_actors_culled==0);
 assert(cnk_test_gl.invalid==0&&cnk_test_gl.matrix_stack_depth==0);
 /* A live bomb outside the original camera frustum must not cost a Vita draw
  * or change the positions of the three actors still visible. */
 game.session.items.bombs[1]=(cnk_item_bomb){{1000,0,1},0,2,0,1};
 cnk_test_gl_reset(game.render.vertex_count);
 assert(!cnk_vitagl_draw_game_frame_with_items_culled(&game,NULL,NULL,NULL,NULL,NULL,NULL,&result));
 assert(result.item_actors==3&&result.item_draw_calls==3&&result.item_actors_culled==1);
 game.session.items.bombs[1].active=0;
 /* Corrupt position is rejected before ANY track draw, not mid-render. */
 game.session.items.missiles[0].position.x=NAN;
 cnk_test_gl_reset(game.render.vertex_count);
 assert(cnk_vitagl_draw_game_frame_with_items_culled(&game,NULL,NULL,NULL,NULL,NULL,NULL,&result)==-7);
 assert(cnk_test_gl.draw_calls==0);
 game.session.items.missiles[0].position.x=1;
 /* Every newly reset/deactivated actor disappears on the next draw. */
 memset(&game.session.items,0,sizeof(game.session.items));
 cnk_test_gl_reset(game.render.vertex_count);
 assert(!cnk_vitagl_draw_game_frame_with_items_culled(&game,NULL,NULL,NULL,NULL,NULL,NULL,&result));
 assert(result.item_draw_calls==0&&result.item_actors==0);
 assert(cnk_test_gl.invalid==0);
 /* Host-only exercise the actual Vita PNG upload, sprite binding and draw
  * pipeline. Chosen image here is SYNTHETIC TEST DATA, not a Zeebo mapping. */
 cnk_png_catalog catalog={0};cnk_vitagl_textures textures={0};
 assert(!cnk_png_catalog_build(&vfs,&catalog) && catalog.count);
 cnk_test_gl_reset(256);
 assert(!cnk_vitagl_textures_init(&textures,&vfs,&catalog,CNK_ITEM_SPRITE_COUNT));
 assert(!cnk_vitagl_textures_upload(&textures,(cnk_texture_binding){CNK_ITEM_SPRITE_MISSILE,catalog.items[0].vfs_index}));
 assert(cnk_test_gl.images_uploaded==1 && cnk_test_gl.handles_created==1);
 frame.count=1;
 frame.actors[0]=(cnk_item_visual){{1,0,2},1,CNK_ITEM_SPRITE_MISSILE};
 assert(!cnk_vitagl_draw_items(&frame,&cam,uploaded,&textures,&calls));
 assert(calls==1 && cnk_test_gl.textured_calls==1);
 cnk_vitagl_textures_free(&textures);
 assert(cnk_test_gl.handles_deleted==1 && cnk_test_gl.invalid==0);
 cnk_png_catalog_free(&catalog);
 cnk_game_unload(&game);fufs_close(&vfs);
 puts("PASS: Vita source billboard draw, verified sprite callback, invalid input preflight, live weapon GPU lifecycle");
 return 0;
}
