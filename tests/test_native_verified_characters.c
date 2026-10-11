/* Three SOURCE-ASSISTED character texture->3DOB->VitaGL integrations.
 * Neither a Vita hardware test nor a proof of original kart/driver mapping. */
#include "cnk_verified_characters.h"
#include "cnk_vitagl_objects.h"
#include "vitaGL.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog cat={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&cat)==0);
 assert(cnk_character_original_evidence((cnk_character_identity)-1)==NULL);
 assert(cnk_character_original_evidence((cnk_character_identity)CNK_CHARACTER_COUNT)==NULL);
 cnk_camera camera={0};
 memcpy(camera.view,identity,sizeof(identity));
 memcpy(camera.projection,identity,sizeof(identity));
 for(int i=0;i<CNK_CHARACTER_COUNT;i++){
  const cnk_character_evidence *ev=cnk_character_original_evidence((cnk_character_identity)i);
  assert(ev && ev->name && ev->original_material_name);
  assert(vfs.entries[ev->model_index].id==ev->model_id);
  assert(vfs.entries[ev->png_index].id==ev->png_id);
  cnk_character_asset original={0};
  cnk_test_gl_reset(0);
  assert(cnk_character_original_load(&vfs,&cat,(cnk_character_identity)i,&original)==0);
  assert(original.skin.ready && original.skin.mesh.batch_count>0);
  assert(cnk_test_gl.images_uploaded==1 && cnk_test_gl.nonzero_texels>0);
  assert(cnk_test_gl.last_upload_width==256 && cnk_test_gl.last_upload_height==256);
  assert(cnk_character_original_resolve(&original,original.skin.selectors[0])!=0 ||
         original.skin.mesh.batch_count>1);
  assert(cnk_character_original_resolve(&original,UINT32_MAX)==0);
  cnk_test_gl_reset(original.skin.mesh.vertex_count);
  unsigned calls=0;
  assert(cnk_vitagl_draw_object(&original.skin.mesh,&camera,identity,
       cnk_character_original_resolve,&original,&calls)==0);
  assert(calls==original.skin.mesh.batch_count);
  assert(cnk_test_gl.textured_calls>=1 && !cnk_test_gl.invalid);
  assert(cnk_test_gl.matrix_stack_depth==0);
  cnk_character_original_free(&original);
  assert(!original.skin.ready);
  /* Swapped/mutated identities cannot silently pass. */
  uint32_t old=vfs.entries[ev->png_index].id;
  vfs.entries[ev->png_index].id=0;
  cnk_test_gl_reset(0);
  assert(cnk_character_original_load(&vfs,&cat,(cnk_character_identity)i,&original)<0);
  assert(cnk_test_gl.handles_created==0);
  vfs.entries[ev->png_index].id=old;
  cnk_test_gl_reset(0);
  cnk_test_gl.fail_next_upload=1;
  assert(cnk_character_original_load(&vfs,&cat,(cnk_character_identity)i,&original)<0);
  assert(cnk_test_gl.handles_created==1 && cnk_test_gl.handles_deleted==1);
  assert(!original.skin.ready && !original.skin.mesh.vertices);
  printf("PASS original %s model #%u skin #%u: upload, draw, integrity, GL rollback\n",
         ev->name,ev->model_index,ev->png_index);
 }
 /* Multi-character load is all-or-nothing, even on the LAST GPU upload. */
 cnk_character_pack pack={0};
 cnk_test_gl_reset(0);
 assert(cnk_character_pack_load(&vfs,&cat,&pack)==0);
 assert(pack.ready && cnk_test_gl.images_uploaded==CNK_CHARACTER_COUNT);
 cnk_character_pack_free(&pack);
 assert(!pack.ready && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 cnk_test_gl_reset(0);
 cnk_test_gl.fail_upload_on_number=3;
 assert(cnk_character_pack_load(&vfs,&cat,&pack)<0);
 assert(!pack.ready && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* One corrupted PNG ID must be rejected before touching the GPU. */
 unsigned j=cnk_character_original_evidence(CNK_CHARACTER_COCO)->png_index;
 uint32_t keep=vfs.entries[j].id;
 vfs.entries[j].id=0;
 cnk_test_gl_reset(0);
 assert(cnk_character_pack_load(&vfs,&cat,&pack)<0);
 assert(cnk_test_gl.handles_created==0);
 vfs.entries[j].id=keep;
 cnk_png_catalog_free(&cat);fufs_close(&vfs);
 puts("PASS 3 original character models/textures; NO kart identity or gameplay inferred");
 return 0;
}
