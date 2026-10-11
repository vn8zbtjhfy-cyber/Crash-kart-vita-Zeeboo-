#define _POSIX_C_SOURCE 200809L
#include "cnk_gallery_skins.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
static void put(const char *path,const char *source){
 FILE *p=fopen(path,"wb");assert(p&&fputs(source,p)>=0&&!fclose(p));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog png={0};cnk_source_kart_gallery g={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(!cnk_png_catalog_build(&vfs,&png));
 assert(!cnk_source_kart_gallery_load(&vfs,&g));
 char path[]="/tmp/cnk81_gallery_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 /* Artificial association: TEXL name from original model, PNG index from
  * original container, but their relationship is NOT historically proven. */
 put(path,"# SYNTHETIC TEST ONLY; NEVER SHIP AS HISTORICAL ZEEBO MAPPING\n"
          "0\tNeoSprint01.png\t2\t01d12af6\n"
          "0\tWheel_t.png\t93\t31abc114\n");
 cnk_gallery_skins skins={0};
 cnk_test_gl_reset(UINT32_MAX);
 assert(!cnk_gallery_skins_load(path,&vfs,&png,&g,&skins));
 assert(skins.count==1);
 assert(cnk_test_gl.handles_created==2&&cnk_test_gl.images_uploaded==2);
 cnk_gallery_material_binding binds[CNK_SOURCE_KART_FAMILIES]={{0}};
 for(unsigned lod=0;lod<CNK_SOURCE_KART_LODS;lod++){
  assert(!cnk_gallery_skins_bind(&skins,lod,binds));
  assert(binds[0].resolve==cnk_gallery_skin_resolve);
  for(unsigned f=1;f<CNK_SOURCE_KART_FAMILIES;f++)assert(!binds[f].resolve);
  assert(binds[0].resolve(binds[0].context,1)!=0);
  assert(binds[0].resolve(binds[0].context,UINT32_MAX)==0);
  unsigned before=cnk_test_gl.textured_calls,calls=0;
  assert(!cnk_source_kart_gallery_draw_mapped(&g,lod,binds,&calls));
  assert(calls>=CNK_SOURCE_KART_FAMILIES);
  assert(cnk_test_gl.textured_calls>before);
  assert(!cnk_test_gl.invalid&&!cnk_test_gl.matrix_stack_depth);
 }
 assert(cnk_gallery_skins_bind(&skins,3,binds)<0);
 cnk_gallery_skins_free(&skins);
 assert(cnk_test_gl.handles_deleted==2);
 /* Invalid group/textures must fail BEFORE the first GPU upload. */
 put(path,"0\tNotARealSourceName.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(!cnk_test_gl.handles_created&&!skins.count);
 put(path,"0\tNeoSprint01.png\t2\t01d12af7\n");
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(!cnk_test_gl.handles_created);
 put(path,"0\tNeoSprint01.png\t2\t01d12af6\n"
          "0\tNeoSprint01.png\t2\t01d12af6\n");
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(!cnk_test_gl.handles_created);
 /* A syntactically valid candidate PNG still requires a source TEXL match. */
 put(path,"0\tWheel_t.png\t93\t31abc114\n");
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(!cnk_test_gl.handles_created);
 /* Any failure after partial GPU creation cleans up allocated texture IDs. */
 put(path,"0\tNeoSprint01.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);cnk_test_gl.fail_next_upload=1;
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(cnk_test_gl.handles_created==1&&cnk_test_gl.handles_deleted==1);
 assert(!skins.count);
 /* Failure on second image must roll back the first successfully uploaded
  * texture and the second failed handle before publishing any binding. */
 put(path,"0\tNeoSprint01.png\t2\t01d12af6\n"
          "0\tWheel_t.png\t93\t31abc114\n");
 cnk_test_gl_reset(0);cnk_test_gl.fail_upload_on_number=2;
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(cnk_test_gl.images_uploaded==2);
 assert(cnk_test_gl.handles_created==2&&cnk_test_gl.handles_deleted==2);
 assert(!skins.count);
 /* A later family with an invalid TEXL name cannot produce partial GPU
  * state for the already-valid preceding family. */
 put(path,"0\tNeoSprint01.png\t2\t01d12af6\n"
          "1\tUnverifiedRacerBody.png\t2\t01d12af6\n");
 cnk_test_gl_reset(0);
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)<0);
 assert(!cnk_test_gl.handles_created&&!skins.count);
 assert(!remove(path));
 assert(cnk_gallery_skins_load(path,&vfs,&png,&g,&skins)==-2);
 cnk_source_kart_gallery_free(&g);cnk_png_catalog_free(&png);fufs_close(&vfs);
 puts("PASS: original 18 vehicle meshes + opt-in test-only PNG textures; strict preflight, shared LOD resources, GPU rollback");
 return 0;
}
