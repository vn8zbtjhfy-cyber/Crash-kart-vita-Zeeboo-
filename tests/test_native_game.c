#include "cnk_game.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int closef(float a,float b){return fabsf(a-b)<0.0005f;}
int main(int argc,char **argv){
    assert(argc==2);
    fufs_file vfs={0};
    assert(fufs_open(argv[1],&vfs)==0);
    cnk_game a={0},b={0};
    cnk_driver_input accelerate={1,0,0};
    assert(cnk_game_load(&a,&vfs,vfs.count,3,2)<0);
    assert(cnk_game_load(&a,&vfs,0,3,2)<0);
    assert(!a.loaded);
    uint32_t successful=0,rejected=0,chosen=0;
    for(uint32_t i=0;i<vfs.count;i++){
        unsigned char *raw=NULL;size_t len=0;
        assert(fufs_extract(&vfs,i,&raw,&len)==0);
        int scene=(len>=12 && !memcmp(raw,"PIFF",4) && !memcmp(raw+8,"SCEN",4));
        free(raw);
        if(!scene)continue;
        int rc=cnk_game_load(&a,&vfs,i,3,2);
        if(rc){
            rejected++;
            assert(!a.loaded);
            continue;
        }
        if(!chosen)chosen=i;
        assert(a.geometry.triangle_count>0);
        assert(a.render.triangle_count==a.geometry.triangle_count);
        assert(a.render.vertex_count==a.geometry.triangle_count*3);
        assert(a.render.batch_count>0);
        assert(a.collision.mesh==&a.geometry);
        cnk_vec3 actual_spawn={0};float original_heading=0;
        assert(cnk_scene_start_pose(&a.scene,&actual_spawn,&original_heading)==0);
        assert(isfinite(original_heading));
        cnk_nav_projection spawn_nearest={0};
        assert(cnk_nav_nearest(a.session.route,actual_spawn,1,&spawn_nearest)==0);
        assert(spawn_nearest.distance<20.0f);
        assert(a.session.source==&a.scene);
        assert(a.session.racers[0].race.track==&a.session.track);
        assert(a.session.camera.projection[0]!=0);
        cnk_game_hud h={0};
        assert(cnk_game_hud_read(&a,&h)==0);
        assert(h.lap==0&&h.total_laps==2&&h.competitors==3);
        assert(cnk_game_update(&a,accelerate,251000)<0);
        assert(cnk_game_update(&a,accelerate,0)==0);
        assert(cnk_game_update(&a,accelerate,16667)==1);
        uint32_t events=UINT32_MAX;
        assert(cnk_game_take_events(&a,0,&events)==0);
        assert((events & ~(CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH))==0);
        assert(cnk_game_take_events(&a,0,&events)==0&&events==0);
        assert(cnk_game_take_events(&a,CNK_MAX_RACERS,&events)<0);
        assert(a.simulation_frames==1);
        assert(cnk_game_hud_read(&a,&h)==0&&h.elapsed_ms==16);
        assert(cnk_game_pause(&a,1)==0);
        assert(cnk_game_update(&a,accelerate,250000)==0);
        assert(a.simulation_frames==1&&h.elapsed_ms==16);
        assert(cnk_game_pause(&a,0)==0);
        assert(cnk_game_update(&a,accelerate,16667)==1);
        assert(a.simulation_frames==2);
        printf("[SCEN %u] render %u triangles, %u batches; collision refs=%u\n",
               i,a.render.triangle_count,a.render.batch_count,a.collision.count);
        cnk_game_unload(&a);
        assert(!a.loaded&&a.scene.vertices==NULL&&a.render.vertices==NULL);
        successful++;
    }
    assert(successful==12 && rejected==2 && chosen!=0);
    assert(cnk_game_load(&a,&vfs,chosen,3,2)==0);
    assert(cnk_game_load(&b,&vfs,chosen,3,2)==0);
    /* Same 2 seconds of user input split into 120 or 60 video frames.
     * The CPU physics/race state must not depend on the render refresh. */
    for(unsigned i=0;i<120;i++)assert(cnk_game_update(&a,accelerate,16667)==1);
    for(unsigned i=0;i<60;i++)assert(cnk_game_update(&b,accelerate,33334)==2);
    assert(a.simulation_frames==120&&b.simulation_frames==120);
    for(unsigned k=0;k<3;k++){
        const cnk_kart *p=&a.session.racers[k].kart,*q=&b.session.racers[k].kart;
        assert(closef(p->position.x,q->position.x));
        assert(closef(p->position.z,q->position.z));
        assert(closef(p->position.y,q->position.y));
        assert(closef(p->speed,q->speed));
        assert(a.session.racers[k].race.completed_laps==b.session.racers[k].race.completed_laps);
    }
    cnk_game_hud h={0};
    assert(!cnk_game_hud_read(&a,&h));
    assert(h.elapsed_ms==2000);
    cnk_game_unload(&a);cnk_game_unload(&b);
    /* Full route-driven end-to-end CPU race test. Rendering remains untested
     * on PS Vita. Original physics and opponents are not recovered yet. */
    static const uint32_t tracks[12]={37,39,42,69,73,77,207,211,213,295,298,300};
    for(unsigned t=0;t<12;t++){
        assert(cnk_game_load(&a,&vfs,tracks[t],3,2)==0);
        unsigned frames=0;
        for(;frames<36000;frames++){
            cnk_driver_input p={0};
            if(!a.session.racers[0].race.finished)
                assert(cnk_ai_controls(&a.session.racers[0].ai,
                       &a.session.racers[0].kart,&a.session.tuning,&p)==0);
            assert(cnk_game_update(&a,p,16667)>0);
            if(a.session.racers[0].race.finished&&
               a.session.racers[1].race.finished&&
               a.session.racers[2].race.finished)break;
        }
        assert(frames<36000);
        for(unsigned k=0;k<3;k++){
            assert(a.session.racers[k].race.finished);
            assert(a.session.racers[k].race.completed_laps==2);
        }
        uint32_t events=0;
        assert(cnk_game_take_events(&a,0,&events)==0);
        assert((events&(CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH))==
               (CNK_RACE_GATE|CNK_RACE_LAP|CNK_RACE_FINISH));
        assert(cnk_game_take_events(&a,0,&events)==0&&events==0);
        printf("[END TO END %u] 3 racers, 2 laps, %u frames\n",tracks[t],frames);
        cnk_game_unload(&a);
    }
    fufs_close(&vfs);
    printf("GAME RUNTIME: %u scenes loaded, %u scenes rejected (source-dependent); deterministic 60Hz clock, pause, GPU data, collision, ranking, HUD OK\n",successful,rejected);
    puts("PASS: portable native game-loop integration (NOT a complete playable port).");
    return 0;
}
