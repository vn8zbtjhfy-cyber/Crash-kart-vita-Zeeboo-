#include "cnk_source_kart_gallery.h"
#include "vitaGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};
 assert(!fufs_open(argv[1],&vfs));
 cnk_source_kart_gallery gallery={0};
 assert(!cnk_source_kart_gallery_load(&vfs,&gallery));
 assert(gallery.ready);
 for(unsigned family=0;family<CNK_SOURCE_KART_FAMILIES;family++)
  for(unsigned lod=0;lod<CNK_SOURCE_KART_LODS;lod++){
   const cnk_object_mesh *m=&gallery.mesh[family][lod];
   assert(m->vertices && m->vertex_count && m->batch_count);
   assert(m->triangle_count>0);
   cnk_camera c={0};
   assert(!cnk_source_kart_gallery_camera(m,&c));
   assert(isfinite(c.eye.x) && isfinite(c.eye.z));
   cnk_camera rotated={0};
   assert(!cnk_source_kart_gallery_camera_at(m,1.5707963268f,&rotated));
   /* rotation keeps distance to the true original mesh center */
   float cx=(m->bbox_min[0]+m->bbox_max[0])*0.5f;
   float cz=(m->bbox_min[2]+m->bbox_max[2])*0.5f;
   float d0=hypotf(c.eye.x-cx,c.eye.z-cz);
   float d1=hypotf(rotated.eye.x-cx,rotated.eye.z-cz);
   assert(fabsf(d1-d0)<d0*0.0001f);
   assert(fabsf(rotated.eye.x-c.eye.x)+fabsf(rotated.eye.z-c.eye.z)>0.001f);
   assert(cnk_source_kart_gallery_camera_at(m,NAN,&rotated)<0);
  }
 for(unsigned lod=0;lod<CNK_SOURCE_KART_LODS;lod++){
  cnk_test_gl_reset(UINT32_MAX);
  unsigned calls=0;
  assert(!cnk_source_kart_gallery_draw(&gallery,lod,&calls));
  assert(calls>=CNK_SOURCE_KART_FAMILIES);
  assert(cnk_test_gl.draw_calls==calls);
  assert(cnk_test_gl.textured_calls==0);
  unsigned orbit_calls=0;
  cnk_test_gl_reset(UINT32_MAX);
  assert(!cnk_source_kart_gallery_draw_mapped_at(&gallery,lod,1.04719755f,NULL,&orbit_calls));
  assert(orbit_calls==calls && cnk_test_gl.draw_calls==orbit_calls);
  assert(!cnk_test_gl.invalid&&!cnk_test_gl.matrix_stack_depth);
 }
 cnk_source_kart_gallery corrupted=gallery;
 corrupted.mesh[4][1].bbox_min[0]=NAN;
 unsigned calls=77;
 cnk_test_gl_reset(0);
 assert(cnk_source_kart_gallery_draw(&corrupted,1,&calls)<0);
 assert(calls==0 && cnk_test_gl.draw_calls==0);
 assert(cnk_source_kart_gallery_draw(&gallery,3,&calls)<0);
 cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_source_kart_gallery_draw_mapped_at(&gallery,1,NAN,NULL,&calls)<0);
 assert(cnk_test_gl.draw_calls==0);
 /* Bad FUFS identification must reject the entire 18-model collection;
  * it must not substitute any plausible-looking unverified vehicle. */
 fufs_file altered=vfs;
 altered.entries=malloc(sizeof(*vfs.entries)*vfs.count);
 assert(altered.entries);
 memcpy(altered.entries,vfs.entries,sizeof(*vfs.entries)*vfs.count);
 altered.entries[399].id^=1u;
 cnk_source_kart_gallery rejected={0};
 assert(cnk_source_kart_gallery_load(&altered,&rejected)==-2);
 assert(!rejected.ready);
 for(unsigned i=0;i<CNK_SOURCE_KART_FAMILIES;i++)
  for(unsigned k=0;k<CNK_SOURCE_KART_LODS;k++)
   assert(!rejected.mesh[i][k].vertices);
 free(altered.entries);
 cnk_source_kart_gallery_free(&gallery);
 assert(!gallery.ready);
 fufs_close(&vfs);
 puts("PASS: six ROM vehicle families, 18 verified 3DOB LOD meshes; untextured GPU preview + atomic invalid rejection");
 return 0;
}
