/* End-to-end ORIGINAL Crash 3DOB #19 + sourced Crash skin PLZP #365 ->
 * decoded RGBA -> native Vita GL upload -> exact POLY batch -> draw.
 * Recorded GL on PC only; not a Vita hardware test nor a playable race. */
#include "cnk_verified_crash.h"
#include "cnk_vitagl_objects.h"
#include "vitaGL.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog catalog={0};cnk_verified_crash character={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&catalog)==0);
 cnk_test_gl_reset(0);
 assert(cnk_verified_crash_load(&vfs,&catalog,&character)==0);
 assert(character.original.ready && character.original.mesh.batch_count==3);
 assert(cnk_test_gl.images_uploaded==1 && cnk_test_gl.last_upload_width==256 &&
        cnk_test_gl.last_upload_height==256 && cnk_test_gl.nonzero_texels>0);
 assert(cnk_verified_crash_resolve(&character,character.original.selectors[0])!=0);
 assert(cnk_verified_crash_resolve(&character,UINT32_MAX)==0);
 cnk_camera camera={0};memcpy(camera.projection,identity,sizeof(identity));
 memcpy(camera.view,identity,sizeof(identity));
 unsigned calls=0;
 uint32_t verts=character.original.mesh.vertex_count;
 cnk_test_gl_reset(verts);
 assert(cnk_vitagl_draw_object(&character.original.mesh,&camera,identity,
                              cnk_verified_crash_resolve,&character,&calls)==0);
 assert(calls==3 && cnk_test_gl.draw_calls==3 && cnk_test_gl.textured_calls==1 &&
        cnk_test_gl.untextured_calls==2 && !cnk_test_gl.invalid);
 assert(cnk_test_gl.matrix_stack_depth==0);
 cnk_verified_crash_free(&character);
 assert(!character.original.ready);
 /* Generic loader must reject name/ID substitutions and duplicate source
  * selectors, all before any GPU upload. */
 cnk_object_skin generic={0};
 cnk_object_skin_reference good={"Crash.png",365u,0xee898141u};
 cnk_object_skin_reference badname={"WrongSkin.png",365u,0xee898141u};
 cnk_object_skin_reference badid={"Crash.png",365u,0xee898140u};
 cnk_object_skin_reference dup[2]={good,good};
 cnk_test_gl_reset(0);
 assert(cnk_object_skin_load(&vfs,&catalog,19u,0x0845c03fu,&badname,1,&generic)<0);
 assert(cnk_object_skin_load(&vfs,&catalog,19u,0x0845c03fu,&badid,1,&generic)<0);
 assert(cnk_object_skin_load(&vfs,&catalog,19u,0x0845c03fu,dup,2,&generic)<0);
 assert(cnk_object_skin_load(&vfs,&catalog,19u,0x0845c03eu,&good,1,&generic)<0);
 assert(cnk_test_gl.handles_created==0);

 /* Reject altered record IDs *before* touching GPU. */
 uint32_t original=vfs.entries[365].id;
 vfs.entries[365].id=0;
 cnk_test_gl_reset(0);
 assert(cnk_verified_crash_load(&vfs,&catalog,&character)<0);
 assert(cnk_test_gl.handles_created==0);
 vfs.entries[365].id=original;
 /* Simulate Vita GPU refusing texture upload: release GL handle + model. */
 cnk_test_gl_reset(0);
 cnk_test_gl.fail_next_upload=1;
 assert(cnk_verified_crash_load(&vfs,&catalog,&character)<0);
 assert(cnk_test_gl.handles_created==1 && cnk_test_gl.handles_deleted==1);
 assert(!character.original.ready && !character.original.mesh.vertices && !character.original.textures.handles);
 cnk_png_catalog_free(&catalog); fufs_close(&vfs);
 puts("PASS original Crash texture #365 uploaded and drawn on source model #19; robust cleanup on GPU error; no vehicle identity inferred");
 return 0;
}
