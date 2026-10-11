#include "cnk_texture_map.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Candidate-only tests. NO guessed image/name link is exported as authentic. */
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog cat={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&cat)==0);
 assert(cat.count==317);
 cnk_texture_reference verified_internally[1024];
 size_t count=0,matched_scene_slots=0,scenes=0;
 /* Build synthetic but internally valid comparisons from the ACTUAL same
  * literal texture name in two different SCENEs. These are NOT game bindings. */
 char seen[512][251];unsigned seen_count=0;
 for(uint32_t i=0;i<vfs.count;i++){
  unsigned char *raw=NULL;size_t n=0;
  assert(fufs_extract(&vfs,i,&raw,&n)==0);
  if(n>=12&&!memcmp(raw+8,"SCEN",4)){
   cnk_scene sc={0};assert(cnk_parse_scene(raw,n,&sc)==0);scenes++;
   for(uint32_t slot=0;slot<sc.texture_count;slot++){
    const char *name=sc.texture_names[slot];
    if(!name[0])continue;
    int previous=-1;
    for(size_t j=0;j<count;j++)
     if(!strcmp(verified_internally[j].original_texture_name,name)){
      previous=(int)j;break;
     }
    if(previous>=0){matched_scene_slots++;continue;}
    /* To protect against candidate name memory lifetime, copy the literal
     * into fixed storage. A name is source metadata, not a guessed PNG ID. */
    if(count>=512)continue;
    assert(strlen(name)<=250);
    strcpy(seen[seen_count],name);
    const cnk_png_resource *p=&cat.items[seen_count%cat.count];
    verified_internally[count++]=(cnk_texture_reference){
     i,vfs.entries[i].id,(uint16_t)slot,
     seen[seen_count],p->vfs_index,p->vfs_id
    };
    seen_count++;
   }
   cnk_scene_free(&sc);
  }
  free(raw);
 }
 assert(scenes==14&&count>100&&matched_scene_slots>100);
 assert(cnk_texture_map_consistency(verified_internally,count)==0);
 /* Exact same original name across scenes with same image: consistent. */
 cnk_texture_reference two[2]={verified_internally[0],verified_internally[0]};
 two[1].scene_vfs_index++;
 two[1].scene_resource_id++;
 two[1].texture_slot++;
 assert(cnk_texture_map_consistency(two,2)==0);
 /* Same name but different original PNG ID: contradiction, not allowed. */
 two[1].png_vfs_index=cat.items[1].vfs_index;
 two[1].png_resource_id=cat.items[1].vfs_id;
 assert(cnk_texture_map_consistency(two,2)==-2);
 /* Different name but two indices claiming same VFS ID: contradiction. */
 two[1].original_texture_name="other.png";
 two[1].png_resource_id=two[0].png_resource_id;
 assert(cnk_texture_map_consistency(two,2)==-3);
 two[1]=two[0];
 assert(cnk_texture_map_consistency(two,2)==-4);
 assert(cnk_texture_map_consistency(NULL,1)==-1);
 assert(cnk_texture_map_consistency(NULL,0)==0);
 assert(cnk_texture_map_consistency(two,8193)==-1);
 printf("PASS: %u scenes, %zu unique original names tested; cross-scene consistency rejects contradictory candidate mappings. No authenticity inferred.\n",scenes,count);
 cnk_png_catalog_free(&cat);fufs_close(&vfs);
 return 0;
}
