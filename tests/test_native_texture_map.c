#include "cnk_texture_map.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog cat={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&cat)==0);
 assert(cat.count==317);
 uint32_t scenes=0,slots=0;
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *blob=NULL;size_t size=0;
  assert(fufs_extract(&vfs,i,&blob,&size)==0);
  if(size>=12&&!memcmp(blob+8,"SCEN",4)){
   cnk_scene s={0};assert(cnk_parse_scene(blob,size,&s)==0);
   scenes++;slots+=s.texture_count;
   /* Valid empty map must never infer any PNG. */
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,NULL,0,NULL,0)==0);
   /* These mappings are artificial NEGATIVE/INPUT tests only; they are
      never exported as authentic associations. */
   uint32_t slot=0;
   while(slot<s.texture_count&&!s.texture_names[slot][0])slot++;
   assert(slot<s.texture_count);
   const cnk_png_resource *p=&cat.items[0];
   cnk_texture_reference r={i,vfs.entries[i].id,(uint16_t)slot,
      s.texture_names[slot],p->vfs_index,p->vfs_id};
   cnk_texture_binding b={65535,0xdeadbeef};
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,&r,1,&b,1)==0);
   assert(b.texture_index==slot&&b.vfs_index==p->vfs_index);
   cnk_texture_binding untouched={65535,0xdeadbeef};
   r.scene_resource_id^=1;
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,&r,1,&untouched,1)==-2);
   r.scene_resource_id^=1;
   assert(untouched.texture_index==65535&&untouched.vfs_index==0xdeadbeef);
   r.original_texture_name="different.png";
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,&r,1,&untouched,1)==-3);
   r.original_texture_name=s.texture_names[slot];
   r.png_resource_id^=1;
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,&r,1,&untouched,1)==-4);
   r.png_resource_id^=1;
   cnk_texture_reference duplicated[2]={r,r};
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,duplicated,2,&untouched,1)==-1);
   cnk_texture_binding outs[2]={{65535,5},{65535,6}};
   assert(cnk_texture_map_validate(&s,i,&vfs,&cat,duplicated,2,outs,2)==-5);
   assert(outs[0].texture_index==65535&&outs[1].texture_index==65535);
   cnk_scene_free(&s);
  }
  free(blob);
 }
 assert(scenes==14&&slots==425);
 printf("PASS: validated %u original scenes, %u texture slots; wrong IDs, missing names and collisions rejected. NO texture assignments inferred.\n",scenes,slots);
 cnk_png_catalog_free(&cat);fufs_close(&vfs);return 0;
}
