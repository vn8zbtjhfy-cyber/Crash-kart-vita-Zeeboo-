/* Full existing game-session -> actual Vita draw bridge integration test.
 * Only GL calls are recorded on host; original Vita execution is unverified. */
#include "cnk_vitagl_game_frame.h"
#include "cnk_track_catalog.h"
#include "cnk_frustum.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static unsigned missing(void *ctx,uint16_t resource){
    ++*(unsigned*)ctx;
    (void)resource;
    return 0; /* Honest: original PNG/material associations are not known. */
}
static void empty_frame_rejected(void){
    cnk_game empty={0};
    cnk_vitagl_frame_result out={0};
    cnk_test_gl_reset(0);
    assert(cnk_vitagl_draw_game_frame(&empty,NULL,NULL,&out)==-1);
    assert(cnk_test_gl.draw_calls==0 && cnk_test_gl.matrix_loads==0);
}
int main(int argc,char **argv){
    assert(argc==2);
    empty_frame_rejected();
    fufs_file vfs={0};
    assert(fufs_open(argv[1],&vfs)==0);
    cnk_track_catalog catalog={0};
    assert(cnk_track_catalog_build(&vfs,&catalog)==0);
    assert(catalog.race_count==12);
    uint64_t total_draws=0,total_triangles=0;
    for(uint32_t i=0;i<catalog.race_count;i++){
        cnk_game game={0};
        assert(cnk_game_load_catalog_race(&game,&catalog,i,3,2)==0);
        assert(game.source_spawn_enabled==1);
        assert(game.session.racer_count==3);
        assert(game.render.triangle_count>0);
        cnk_vitagl_frame_result frame={0};
        unsigned lookups=0;
        cnk_test_gl_reset(game.render.vertex_count);
        assert(cnk_vitagl_draw_game_frame(&game,missing,&lookups,&frame)==0);
        assert(!cnk_test_gl.invalid);
        assert(frame.simulation_frames==0 && frame.race.rank>=1 && frame.race.rank<=3);
        assert(frame.race.competitors==3 && frame.race.total_laps==2);
        assert(frame.visible_track_triangles==game.render.triangle_count);
        assert(frame.track_draw_calls==game.render.batch_count);
        assert(cnk_test_gl.drawn_vertices==game.render.vertex_count);
        assert(cnk_test_gl.draw_calls==frame.track_draw_calls);
        assert(cnk_test_gl.untextured_calls==frame.track_draw_calls);
        assert(lookups==frame.track_draw_calls);
        assert(cnk_test_gl.matrix_stack_depth==0);
        /* Production Vita path may skip only batches known fully off-screen. */
        cnk_vitagl_frame_result culled={0};
        uint32_t expected_tris=0;
        unsigned expected_draws=0;
        for(uint32_t j=0;j<game.render.batch_count;j++){
            const cnk_render_batch *b=&game.render.batches[j];
            int visible=cnk_batch_in_frustum(&game.session.camera,b);
            assert(visible==0||visible==1);
            if(visible && b->vertex_count){
                expected_tris+=b->vertex_count/3u;expected_draws++;
            }
        }
        cnk_test_gl_reset(game.render.vertex_count);
        assert(cnk_vitagl_draw_game_frame_with_racers_culled(&game,NULL,NULL,
                             NULL,&culled)==0);
        assert(!cnk_test_gl.invalid && culled.visible_track_triangles==expected_tris &&
               culled.track_draw_calls==expected_draws &&
               cnk_test_gl.drawn_vertices==expected_tris*3u &&
               culled.simulation_frames==0);
        assert(culled.visible_track_triangles<=frame.visible_track_triangles);
        total_draws+=frame.track_draw_calls;
        total_triangles+=frame.visible_track_triangles;
        /* The actual race steps, its camera must update before a new draw. */
        assert(cnk_game_update(&game,(cnk_driver_input){1,0,0},16667)==1);
        cnk_test_gl_reset(game.render.vertex_count);
        cnk_vitagl_frame_result after={0};
        assert(cnk_vitagl_draw_game_frame(&game,NULL,NULL,&after)==0);
        assert(!cnk_test_gl.invalid);
        assert(after.simulation_frames==1);
        assert(after.track_draw_calls==frame.track_draw_calls);
        /* draw bridge is forbidden to advance physics by itself */
        assert(game.simulation_frames==1);
        cnk_game_unload(&game);
    }
    fufs_close(&vfs);
    printf("PASS: 12 authentic race SCEN loads -> camera -> native VitaGL draw call bridge: %llu batches, %llu track triangles per initial frame; no fake texture IDs.\n",
           (unsigned long long)total_draws,(unsigned long long)total_triangles);
    return 0;
}
