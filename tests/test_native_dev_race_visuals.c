#include "cnk_dev_race_visuals.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv){
 assert(argc==2);
 fufs_file rom={0};assert(!fufs_open(argv[1],&rom));
 cnk_dev_race_visuals v={0};assert(!cnk_dev_race_visuals_load(&rom,&v));
 assert(v.ready);
 for(unsigned i=0;i<CNK_DEV_VISUAL_FAMILIES;i++){
  uint32_t index=0,id=0;
  assert(!cnk_source_kart_gallery_source(i,1,&index,&id));
  assert(rom.entries[index].id==id);
  assert(v.meshes[i].triangle_count>0);
  cnk_racer_visual_attachment att={0};
  att.mesh=&v.meshes[i];att.local_from_mesh=v.local[i];
  assert(!cnk_racer_visual_attachment_valid(&att));
  for(unsigned j=0;j<3;j++){
   double scaled=(v.meshes[i].bbox_max[j]-v.meshes[i].bbox_min[j])*v.local[i][0];
   assert(isfinite(scaled)&&scaled>0&&scaled<=1.81);
  }
 }
 cnk_racer_visual_attachment out[CNK_MAX_RACERS]={{0}};
 assert(!cnk_dev_race_visuals_fill(&v,CNK_MAX_RACERS,out));
 for(unsigned i=0;i<CNK_MAX_RACERS;i++){
  assert(out[i].mesh==&v.meshes[i%CNK_DEV_VISUAL_FAMILIES]);
  assert(out[i].local_from_mesh==v.local[i%CNK_DEV_VISUAL_FAMILIES]);
 }
 /* Existing, verified actor binding must take precedence over preview. */
 const cnk_object_mesh *original=out[0].mesh;
 const float *original_matrix=out[0].local_from_mesh;
 out[0].mesh=&v.meshes[5];out[0].local_from_mesh=v.local[5];
 assert(!cnk_dev_race_visuals_fill(&v,CNK_MAX_RACERS,out));
 assert(out[0].mesh==&v.meshes[5]&&out[0].local_from_mesh==v.local[5]);
 assert(out[1].mesh==&v.meshes[1]);
 assert(original && original_matrix);
 cnk_racer_visual_attachment old[CNK_MAX_RACERS];memcpy(old,out,sizeof(old));
 assert(cnk_dev_race_visuals_fill(&v,CNK_MAX_RACERS+1,out)!=0);
 assert(!memcmp(old,out,sizeof(old)));
 cnk_dev_race_visuals_free(&v);assert(!v.ready);
 assert(cnk_dev_race_visuals_fill(&v,1,out)!=0);
 /* Source IDs are validated, including on malformed metadata. */
 uint32_t index=0,id=0;assert(!cnk_source_kart_gallery_source(0,1,&index,&id));
 uint32_t saved=rom.entries[index].id;rom.entries[index].id^=1;
 assert(cnk_dev_race_visuals_load(&rom,&v)!=0&&!v.ready);
 rom.entries[index].id=saved;
 fufs_close(&rom);
 puts("developer visual fallback: six authentic 3DOB models, no canonical assignment");
 return 0;
}
