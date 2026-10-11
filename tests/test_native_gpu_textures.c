/* Exercise the real Vita texture loader against every ORIGINAL Zeebo PNG.
 * The graphics recorder verifies decoded pixels and upload arguments.
 * It is not an emulation of Zeebo and does not infer any material names. */
#include "fufs.h"
#include "cnk_png.h"
#include "cnk_vitagl_textures.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog cat={0};cnk_vitagl_textures set={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&cat)==0);
 assert(cat.count==317);
 assert(cnk_vitagl_textures_init(&set,&vfs,&cat,(uint32_t)cat.count)==0);
 cnk_test_gl_reset(0);
 uint64_t total_pixels=0;
 for(size_t i=0;i<cat.count;i++){
  cnk_texture_binding bind={(uint16_t)i,cat.items[i].vfs_index};
  uint32_t before=cnk_test_gl.images_uploaded;
  assert(cnk_vitagl_textures_upload(&set,bind)==0);
  assert(cnk_test_gl.images_uploaded==before+1);
  assert(cnk_test_gl.last_upload_width==cat.items[i].image.width);
  assert(cnk_test_gl.last_upload_height==cat.items[i].image.height);
  assert(cnk_vitagl_textures_resolve(&set,(uint16_t)i)!=0);
  assert(cnk_vitagl_textures_upload(&set,bind)==-3); /* no replacement */
  total_pixels+=(uint64_t)cat.items[i].image.width*cat.items[i].image.height;
 }
 assert(cnk_test_gl.images_uploaded==317);
 assert(cnk_test_gl.pixels_uploaded==total_pixels);
 assert(cnk_test_gl.nonzero_texels>0);
 assert(cnk_test_gl.invalid==0);
 assert(cnk_vitagl_textures_resolve(&set,317)==0);
 assert(cnk_vitagl_textures_resolve(NULL,0)==0);
 assert(cnk_vitagl_textures_upload(&set,(cnk_texture_binding){317,cat.items[0].vfs_index})==-1);
 assert(cnk_vitagl_textures_upload(&set,(cnk_texture_binding){0,vfs.count})==-1);
 /* Bad archive ID must be rejected, without touching the GPU or resources. */
 uint32_t id=vfs.entries[cat.items[0].vfs_index].id;
 vfs.entries[cat.items[0].vfs_index].id^=1;
 assert(cnk_vitagl_textures_upload(&set,(cnk_texture_binding){0,cat.items[0].vfs_index})==-2);
 vfs.entries[cat.items[0].vfs_index].id=id;
 cnk_vitagl_textures_free(&set);
 assert(cnk_test_gl.handles_deleted==317);
 assert(!set.handles && !set.count);
 /* A GPU error must destroy the newly allocated handle and leave slot empty. */
 assert(cnk_vitagl_textures_init(&set,&vfs,&cat,1)==0);
 cnk_test_gl.fail_next_upload=1;
 assert(cnk_vitagl_textures_upload(&set,(cnk_texture_binding){0,cat.items[0].vfs_index})==-11);
 assert(!cnk_vitagl_textures_resolve(&set,0));
 assert(cnk_test_gl.handles_deleted==318);
 assert(cnk_vitagl_textures_upload(&set,(cnk_texture_binding){0,cat.items[0].vfs_index})==0);
 assert(cnk_vitagl_textures_resolve(&set,0)!=0);
 cnk_vitagl_textures_free(&set);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.invalid==0);
 printf("PASS GPU texture loader: %zu source PNGs, %llu decoded RGBA pixels, %u uploads, GPU failure cleanup verified\n",
        cat.count,(unsigned long long)total_pixels,cnk_test_gl.images_uploaded);
 cnk_png_catalog_free(&cat);fufs_close(&vfs);
 return 0;
}
