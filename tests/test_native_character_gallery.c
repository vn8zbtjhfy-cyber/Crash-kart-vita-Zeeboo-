#include "cnk_character_gallery.h"
#include "cnk_verified_characters.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog png={0};cnk_character_pack pack={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(!cnk_png_catalog_build(&vfs,&png));
 cnk_test_gl_reset(0);
 assert(!cnk_character_pack_load(&vfs,&png,&pack));
 cnk_test_gl_reset(UINT32_MAX);
 unsigned n=99;
 assert(!cnk_character_gallery_draw(&pack,&n));
 assert(n>=3 && cnk_test_gl.textured_calls>=3 && !cnk_test_gl.invalid);
 assert(!cnk_test_gl.matrix_stack_depth);
 cnk_test_gl_reset(UINT32_MAX);
 unsigned turning=0;
 assert(!cnk_character_gallery_draw_at(&pack,2.0943951f,&turning));
 assert(turning==n && cnk_test_gl.draw_calls==turning &&
        cnk_test_gl.textured_calls>=3 && !cnk_test_gl.invalid);
 cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_character_gallery_draw_at(&pack,NAN,&turning)<0);
 assert(cnk_test_gl.draw_calls==0);
 for(unsigned i=0;i<CNK_CHARACTER_COUNT;i++){
   const cnk_object_mesh *m=&pack.characters[i].skin.mesh;
   cnk_camera c={0};
   assert(!cnk_character_gallery_camera(m,&c));
   assert(isfinite(c.eye.x)&&isfinite(c.eye.y)&&isfinite(c.eye.z));
   assert(c.eye.z>c.target.z);
   cnk_camera orbit={0};
   assert(!cnk_character_gallery_camera_at(m,1.5707963268f,&orbit));
   float cx=(m->bbox_min[0]+m->bbox_max[0])*0.5f;
   float cz=(m->bbox_min[2]+m->bbox_max[2])*0.5f;
   assert(fabsf(hypotf(orbit.eye.x-cx,orbit.eye.z-cz)-
                hypotf(c.eye.x-cx,c.eye.z-cz))<0.001f);
   assert(cnk_character_gallery_camera_at(m,INFINITY,&orbit)<0);
 }
 cnk_character_pack damaged=pack;
 damaged.characters[1].skin.mesh.bbox_max[0]=NAN;
 n=123;cnk_test_gl_reset(0);
 assert(cnk_character_gallery_draw(&damaged,&n)<0 && n==0);
 assert(cnk_test_gl.draw_calls==0 && !cnk_test_gl.invalid);
 n=123;cnk_test_gl_reset(0);
 damaged=pack;damaged.characters[2].skin.ready=0;
 assert(cnk_character_gallery_draw(&damaged,&n)<0 && n==0);
 assert(cnk_test_gl.draw_calls==0);
 cnk_character_pack_free(&pack);
 assert(cnk_test_gl.handles_created==0 || cnk_test_gl.handles_deleted==3);
 cnk_png_catalog_free(&png);fufs_close(&vfs);
 puts("PASS original gallery: 3 model+texture renders, camera framing, atomic rejection");
 return 0;
}
