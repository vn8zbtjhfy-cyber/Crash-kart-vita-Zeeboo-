/* Native source 3DOB mesh -> live race kart pose -> vitaGL draw bridge.
 * A SOURCE MODEL SELECTED JUST FOR TESTING IS NOT A VERIFIED KART MODEL.
 * On host, the GL shim merely records API calls; Vita hardware not tested. */
#include "cnk_vitagl_game_frame.h"
#include "cnk_track_catalog.h"
#include "cnk_model.h"
#include "cnk_object_render.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const float identity[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
static void close_enough(float a,float b){assert(fabsf(a-b)<0.002f);}
static void test_matrices(void){
    cnk_kart kart={0}; float m[16]={0};
    kart.position=(cnk_vec3){3.5f,5.0f,-2.0f};
    kart.heading=0;
    assert(cnk_racer_visual_matrix(&kart,identity,m)==0);
    close_enough(m[12],3.5f);close_enough(m[13],5.0f);close_enough(m[14],-2.0f);
    close_enough(m[8],0);close_enough(m[10],1);
    kart.heading=1.5707963268f;
    assert(cnk_racer_visual_matrix(&kart,identity,m)==0);
    close_enough(m[8],1);close_enough(m[10],0);
    close_enough(m[0],0);close_enough(m[2],-1);
    float calibration[16];memcpy(calibration,identity,sizeof(calibration));
    calibration[12]=2;calibration[13]=4;calibration[14]=3;
    assert(cnk_racer_visual_matrix(&kart,calibration,m)==0);
    close_enough(m[12],6.5f);close_enough(m[13],9);close_enough(m[14],-4);
    m[0]=1337;
    calibration[3]=1;
    assert(cnk_racer_visual_matrix(&kart,calibration,m)<0);
    assert(m[0]==1337); /* invalid transform leaves output untouched */
    calibration[3]=0; calibration[6]=NAN;
    assert(cnk_racer_visual_matrix(&kart,calibration,m)<0);
}
static int source_model(const fufs_file *vfs,cnk_object_mesh *out){
    for(uint32_t i=0;i<vfs->count;i++){
        unsigned char *bytes=NULL;size_t len=0;
        assert(fufs_extract(vfs,i,&bytes,&len)==0);
        int match=len>=12 && memcmp(bytes,"PIFF",4)==0 && memcmp(bytes+8,"3DOB",4)==0;
        int rc=1;
        if(match)rc=cnk_object_mesh_from_vfs(vfs,i,out);
        free(bytes);
        if(match)return rc;
    }
    return -1;
}
static void check_poses(const cnk_game *g,uint32_t n){
    assert(cnk_test_gl.matrix_multiplications==n);
    for(uint32_t i=0;i<n;i++){
       const float *m=cnk_test_gl.recorded_model_matrices+i*16;
       const cnk_kart *kart=&g->session.racers[i].kart;
       close_enough(m[12],kart->position.x);
       close_enough(m[13],kart->position.y);
       close_enough(m[14],kart->position.z);
       close_enough(m[8],sinf(kart->heading));
       close_enough(m[10],cosf(kart->heading));
    }
}
int main(int argc,char **argv){
    assert(argc==2);
    test_matrices();
    fufs_file vfs={0};assert(fufs_open(argv[1],&vfs)==0);
    cnk_object_mesh mesh={0};
    assert(source_model(&vfs,&mesh)==0);
    assert(mesh.triangle_count>0);
    cnk_racer_visual_attachment visuals[CNK_MAX_RACERS]={{0}};
    for(unsigned i=0;i<3;i++){
        visuals[i].mesh=&mesh;
        visuals[i].local_from_mesh=identity;
    }
    assert(cnk_racer_visual_attachment_valid(&visuals[0])==0);
    cnk_track_catalog catalog={0};assert(cnk_track_catalog_build(&vfs,&catalog)==0);
    assert(catalog.race_count==12);
    uint64_t total_model_draws=0;
    for(uint32_t track=0;track<catalog.race_count;track++){
        cnk_game game={0};
        assert(cnk_game_load_catalog_race(&game,&catalog,track,3,2)==0);
        uint32_t maxverts=game.render.vertex_count>mesh.vertex_count?
            game.render.vertex_count:mesh.vertex_count;
        cnk_test_gl_reset(maxverts);
        cnk_vitagl_frame_result frame={0};
        assert(cnk_vitagl_draw_game_frame_with_racers(&game,NULL,NULL,visuals,&frame)==0);
        assert(!cnk_test_gl.invalid);
        assert(frame.racer_models_drawn==3);
        assert(frame.racer_triangles==mesh.triangle_count*3);
        assert(frame.racer_draw_calls==mesh.batch_count*3);
        assert(frame.track_draw_calls==game.render.batch_count);
        assert(cnk_test_gl.draw_calls==frame.track_draw_calls+frame.racer_draw_calls);
        assert(cnk_test_gl.matrix_stack_depth==0);
        check_poses(&game,3);
        total_model_draws+=frame.racer_draw_calls;
        /* Before the GPU touches anything, reject malformed mesh references
         * and bad source-model calibration. */
        cnk_object_mesh malformed=mesh;
        cnk_object_batch bad_batch=*mesh.batches;
        malformed.batches=&bad_batch;
        malformed.batch_count=1;
        bad_batch.first_vertex=12;
        visuals[1].mesh=&malformed;
        cnk_test_gl_reset(maxverts);
        assert(cnk_vitagl_draw_game_frame_with_racers(&game,NULL,NULL,visuals,&frame)==-4);
        assert(cnk_test_gl.draw_calls==0&&cnk_test_gl.matrix_loads==0);
        visuals[1].mesh=&mesh;
        float nan_matrix[16];memcpy(nan_matrix,identity,sizeof(nan_matrix));
        nan_matrix[12]=INFINITY;
        visuals[2].local_from_mesh=nan_matrix;
        assert(cnk_vitagl_draw_game_frame_with_racers(&game,NULL,NULL,visuals,&frame)==-4);
        assert(cnk_test_gl.draw_calls==0);
        visuals[2].local_from_mesh=identity;
        /* Moving karts generate new world transforms without changing
         * source 3DOB data, and draw with the updated camera. */
        assert(cnk_game_update(&game,(cnk_driver_input){1,0,0},16667)==1);
        cnk_test_gl_reset(maxverts);
        assert(cnk_vitagl_draw_game_frame_with_racers(&game,NULL,NULL,visuals,&frame)==0);
        assert(!cnk_test_gl.invalid);
        assert(frame.simulation_frames==1);
        check_poses(&game,3);
        /* Without VERIFIED model assignments, preserve track-only result. */
        cnk_test_gl_reset(maxverts);
        assert(cnk_vitagl_draw_game_frame(&game,NULL,NULL,&frame)==0);
        assert(!frame.racer_models_drawn&&!frame.racer_draw_calls&&!frame.racer_triangles);
        assert(cnk_test_gl.matrix_multiplications==0);
        cnk_game_unload(&game);
    }
    fufs_close(&vfs);
    cnk_object_mesh_free(&mesh);
    printf("PASS: source 3DOB attached explicitly to 3 moving racer poses across 12 source tracks; %llu recorded model draws. No historical kart identity inferred.\n",(unsigned long long)total_model_draws);
    return 0;
}
