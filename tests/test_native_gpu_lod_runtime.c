/* Actual source models, runtime camera distance and production GPU draw path.
 * Only test sidecars assign family identity; no historical racer claim. */
#define _POSIX_C_SOURCE 200809L
#include "cnk_racer_assets.h"
#include "cnk_vitagl_game_frame.h"
#include "cnk_track_catalog.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const unsigned families[2][3]={{5,8,12},{26,29,30}};
static const char *matrix="1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1";
static void lod_row(FILE *f,const fufs_file *vfs,unsigned slot,
                    const unsigned refs[3],const char *near_s,const char *far_s){
 fprintf(f,"%u",slot);
 for(unsigned i=0;i<3;i++)
  fprintf(f,"\t%u\t%08x",refs[i],vfs->entries[refs[i]].id);
 fprintf(f,"\t%s\t%s\t%s\n",near_s,far_s,matrix);
}
static void assert_mem_clear(const cnk_racer_assets *a){
 for(unsigned i=0;i<CNK_MAX_RACERS;i++){
  assert(!a->assigned[i]&&!a->lods[i].levels[0].vertices&&!a->meshes[i].vertices);
 }
}
static void draw(const cnk_game *g,const cnk_racer_visual_attachment *vis,
                 unsigned expected_racers,uint32_t expected_triangles){
 uint32_t maxverts=g->render.vertex_count;
 for(unsigned i=0;i<expected_racers;i++)if(vis[i].mesh &&
           vis[i].mesh->vertex_count>maxverts)maxverts=vis[i].mesh->vertex_count;
 cnk_test_gl_reset(maxverts);
 cnk_vitagl_frame_result frame={0};
 assert(cnk_vitagl_draw_game_frame_with_racers_culled(g,NULL,NULL,vis,&frame)==0);
 assert(!cnk_test_gl.invalid);
 assert(frame.racer_models_drawn==expected_racers);
 assert(frame.racer_triangles==expected_triangles);
 assert(cnk_test_gl.matrix_stack_depth==0);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 char path[]="/tmp/cnk_gpu_lod_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *f=fdopen(fd,"wb");assert(f);
 fprintf(f,"# Test-only candidate model family. NOT source-confirmed characters.\n");
 lod_row(f,&vfs,0,families[0],"10","30");
 lod_row(f,&vfs,1,families[1],"8","20");
 fprintf(f,"2\t%u\t%08x\t%s\n",families[0][0],
         vfs.entries[families[0][0]].id,matrix); /* Legacy fixed slot */
 assert(!fclose(f));
 cnk_racer_assets assets={0};
 assert(!cnk_racer_assets_load(path,&vfs,&assets));
 assert(assets.assigned[0]&&assets.assigned[1]&&assets.assigned[2]);
 assert(assets.has_lod[0]&&assets.has_lod[1]&&!assets.has_lod[2]);
 assert(assets.lods[0].levels[0].triangle_count==319);
 assert(assets.lods[1].levels[0].triangle_count==371);
 cnk_racer_visual_attachment vis[CNK_MAX_RACERS]={{0}};
 cnk_racer_assets_attachments(&assets,vis);
 assert(vis[0].mesh==&assets.lods[0].levels[0]);
 assert(vis[2].mesh==&assets.meshes[2]);
 cnk_track_catalog catalog={0};assert(!cnk_track_catalog_build(&vfs,&catalog));
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&catalog,0,3,2));
 /* Force camera distances for deterministic LOD selection; source geometry
  * and source kart positions still load from real Zeebo scene. */
 for(unsigned i=0;i<3;i++)g.session.racers[i].kart.position=(cnk_vec3){0,0,0};
 assert(!cnk_camera_build(&g.session.camera,(cnk_vec3){5,2,-10},
                (cnk_vec3){0,0,0},1.2f,960.f/544.f,0.1f,200.f));
 g.session.camera.eye=(cnk_vec3){5,0,0};
 assert(!cnk_racer_assets_attachments_for_camera(&assets,&g.session,vis));
 assert(vis[0].mesh==&assets.lods[0].levels[0]);
 assert(vis[1].mesh==&assets.lods[1].levels[0]);
 draw(&g,vis,3,319+371+319);
 assert(!cnk_camera_build(&g.session.camera,(cnk_vec3){15,3,-15},
               (cnk_vec3){0,0,0},1.2f,960.f/544.f,0.1f,200.f));
 g.session.camera.eye=(cnk_vec3){15,0,0};
 assert(!cnk_racer_assets_attachments_for_camera(&assets,&g.session,vis));
 assert(vis[0].mesh==&assets.lods[0].levels[1]);
 assert(vis[1].mesh==&assets.lods[1].levels[1]);
 draw(&g,vis,3,141+180+319);
 assert(!cnk_camera_build(&g.session.camera,(cnk_vec3){31,3,-18},
               (cnk_vec3){0,0,0},1.2f,960.f/544.f,0.1f,200.f));
 g.session.camera.eye=(cnk_vec3){31,0,0};
 assert(!cnk_racer_assets_attachments_for_camera(&assets,&g.session,vis));
 assert(vis[0].mesh==&assets.lods[0].levels[2]);
 assert(vis[1].mesh==&assets.lods[1].levels[2]);
 draw(&g,vis,3,77+89+319);
 /* Invalid pose cannot mutate existing output. */
 cnk_racer_visual_attachment saved[CNK_MAX_RACERS];memcpy(saved,vis,sizeof(vis));
 g.session.camera.eye.x=NAN;
 assert(cnk_racer_assets_attachments_for_camera(&assets,&g.session,vis)<0);
 assert(!memcmp(vis,saved,sizeof(vis)));
 g.session.camera.eye=(cnk_vec3){0,0,0};
 g.session.racers[0].kart.position.x=NAN;
 assert(cnk_racer_assets_attachments_for_camera(&assets,&g.session,vis)<0);
 assert(!memcmp(vis,saved,sizeof(vis)));
 cnk_game_unload(&g);
 cnk_racer_assets_free(&assets);
 assert_mem_clear(&assets);
 /* Reject partial sidecars, mismatched source IDs, wrong LOD order, invalid
  * thresholds and duplicate entries without leaking any staged mesh. */
 f=fopen(path,"wb");assert(f);
 lod_row(f,&vfs,0,families[0],"10","30");
 fprintf(f,"1\t5\tffffffff\t8\t%08x\t12\t%08x\t5\t15\t%s\n",
        vfs.entries[8].id,vfs.entries[12].id,matrix);
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);assert_mem_clear(&assets);
 f=fopen(path,"wb");assert(f);
 const unsigned wrong_order[]={12,8,5};
 lod_row(f,&vfs,0,wrong_order,"10","30");
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);assert_mem_clear(&assets);
 f=fopen(path,"wb");assert(f);
 lod_row(f,&vfs,0,families[0],"30","10");
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);assert_mem_clear(&assets);
 f=fopen(path,"wb");assert(f);
 lod_row(f,&vfs,0,families[0],"10","30");
 lod_row(f,&vfs,0,families[0],"10","30");
 assert(!fclose(f));
 assert(cnk_racer_assets_load(path,&vfs,&assets)<0);assert_mem_clear(&assets);
 assert(!remove(path));
 fufs_close(&vfs);
 puts("PASS: real 3DOB models swap LOD at runtime and draw through production Vita GPU path; no character identities or historic switch distances asserted");
 return 0;
}
