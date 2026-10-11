#define _POSIX_C_SOURCE 200809L
/* Entire chain: original 3DOB LOD models + original candidate PNG from ROM,
 * strict REVIEW-REQUIRED test-only association + vitaGL upload + 3 LOD draws.
 * The synthetic mapping is never a historical Zeebo claim or shipped sidecar. */
#include "cnk_kart_skins.h"
#include "cnk_vitagl_objects.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static const char *identity="1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1";
static void put(const char *p,const char *s){
 FILE *fp=fopen(p,"wb");assert(fp);assert(fputs(s,fp)>=0);assert(!fclose(fp));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog png={0};cnk_racer_assets racers={0};
 assert(!fufs_open(argv[1],&vfs));assert(!cnk_png_catalog_build(&vfs,&png));
 assert(vfs.entries[5].id==0x02d15519u&&vfs.entries[8].id==0x0404d06au&&
        vfs.entries[12].id==0x05384bbbu&&vfs.entries[2].id==0x01d12af6u);
 char racerfile[]="/tmp/cnk44_racers_XXXXXX", skinsfile[]="/tmp/cnk44_skins_XXXXXX";
 int fd=mkstemp(racerfile);assert(fd>=0);close(fd);
 fd=mkstemp(skinsfile);assert(fd>=0);close(fd);
 char row[512];
 snprintf(row,sizeof(row),"# TEST ONLY: SOURCE GEOMETRY, SYNTHETIC SLOT / LOD thresholds\n"
   "0\t5\t02d15519\t8\t0404d06a\t12\t05384bbb\t5\t10\t%s\n",identity);
 put(racerfile,row);
 assert(!cnk_racer_assets_load(racerfile,&vfs,&racers));
 assert(racers.assigned[0]&&racers.has_lod[0]);
 put(skinsfile,"# TEST-ONLY wrong association! Not original Zeebo mapping.\n"
     "0\tNeoSprint01.png\t2\t01d12af6\n"
     "0\tWheel_t.png\t93\t31abc114\n");
 cnk_kart_skins skins={0};cnk_test_gl_reset(0);
 assert(!cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins));
 assert(skins.count==1 && skins.slots[0].levels==3);
 assert(cnk_test_gl.handles_created==2&&cnk_test_gl.images_uploaded==2);
 assert(skins.slots[0].texture_count==2);
 assert(cnk_test_gl.nonzero_texels>0);
 cnk_camera camera={0};memcpy(camera.view,matrix,sizeof(matrix));
 memcpy(camera.projection,matrix,sizeof(matrix));
 for(unsigned level=0;level<3;level++){
  cnk_racer_visual_attachment att[CNK_MAX_RACERS]={{0}};
  cnk_racer_assets_attachments(&racers,att);
  att[0].mesh=&racers.lods[0].levels[level];
  assert(!cnk_kart_skins_apply(&skins,att));
  assert(att[0].mesh==&racers.lods[0].levels[level]);
  assert(att[0].resolve_material==cnk_kart_skins_resolve);
  unsigned primary=0,wheel=0;
  for(unsigned k=0;k<skins.slots[0].material_count[level];k++){
   cnk_kart_selector_binding pair=skins.slots[0].materials[level][k];
   if(pair.texture_index==0)primary++;
   if(pair.texture_index==1)wheel++;
   assert(att[0].resolve_material(att[0].material_context,pair.selector)!=0);
  }
  assert(primary>=1);
  assert(level==2?wheel==2:wheel==0);
  assert(att[0].resolve_material(att[0].material_context,UINT32_MAX)==0);
  cnk_test_gl_reset(att[0].mesh->vertex_count);
  unsigned calls=0;
  assert(!cnk_vitagl_draw_object(att[0].mesh,&camera,matrix,
                 att[0].resolve_material,att[0].material_context,&calls));
  assert(calls==att[0].mesh->batch_count&&cnk_test_gl.textured_calls>=1);
  assert(!cnk_test_gl.invalid && !cnk_test_gl.matrix_stack_depth);
 }
 /* All-or-nothing attachment update for an invalid mesh identity. */
 cnk_racer_visual_attachment bad[CNK_MAX_RACERS]={{0}};
 bad[0].mesh=&racers.lods[0].levels[0];bad[0].local_from_mesh=matrix;
 bad[1].mesh=bad[0].mesh;bad[1].resolve_material=cnk_kart_skins_resolve;
 bad[0].resolve_material=cnk_kart_skins_resolve;
 assert(cnk_kart_skins_apply(&skins,bad)<0);
 assert(bad[0].material_context==NULL);
 bad[0].resolve_material=NULL;
 bad[0].mesh=&racers.lods[0].levels[1];
 assert(!cnk_kart_skins_apply(&skins,bad));
 cnk_kart_skins_free(&skins);
 assert(cnk_test_gl.handles_deleted==2);
 /* No accepted association may refer to a misspelled TEXL name. */
 put(skinsfile,"0\tMadeUpBody.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)<0);
 assert(cnk_test_gl.handles_created==0);
 assert(!skins.count);
 /* Correct main body name, but a wrong FUFS ID: reject BEFORE any GPU call. */
 put(skinsfile,"0\tNeoSprint01.png\t2\t01d12af7\n");
 cnk_test_gl_reset(0);
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)<0);
 assert(cnk_test_gl.handles_created==0);
 /* No duplicate mapping for the same slot. */
 put(skinsfile,"0\tNeoSprint01.png\t2\t01d12af6\n"
               "0\tNeoSprint01.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)<0);
 assert(cnk_test_gl.handles_created==0);
 /* Valid associations cannot appear in an unassigned slot. */
 put(skinsfile,"1\tNeoSprint01.png\t2\t01d12af6\n");
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)<0);
 /* Upload failure MUST free every previously allocated GPU handle. */
 put(skinsfile,"0\tNeoSprint01.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);cnk_test_gl.fail_next_upload=1;
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)<0);
 assert(cnk_test_gl.handles_created==1&&cnk_test_gl.handles_deleted==1);
 assert(!skins.count);
 assert(!remove(skinsfile));
 assert(cnk_kart_skins_load(skinsfile,&vfs,&png,&racers,&skins)==-2);
 assert(!remove(racerfile));
 cnk_racer_assets_free(&racers);cnk_png_catalog_free(&png);fufs_close(&vfs);
 puts("PASS: test-only kart PNG binding, 3 authentic LOD meshes, verified TEXL, shared GPU texture, preflight and rollback; NO asserted historical identity");
 return 0;
}
