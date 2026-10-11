#include "cnk_model_material.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const struct {
 uint32_t idx[3];
 const char *name;
} families[6]={
 {{5,8,12},"NeoSprint01.png"},
 {{26,29,30},"LeMans02.png"},
 {{40,43,45},"cocster2.png"},
 {{173,174,178},"rallye.png"},
 {{249,254,257},"CrashStang128_t.png"},
 {{395,399,401},"NeoMini.png"}
};
int main(int argc,char **argv){
 assert(argc>=2);
 fufs_file vfs={0};assert(fufs_open(argv[1],&vfs)==0);
 unsigned named=0,untextured=0,blocked=0,groups=0;
 for(unsigned i=0;i<vfs.count;i++){
  unsigned char *raw=NULL;size_t len=0;
  assert(fufs_extract(&vfs,i,&raw,&len)==0);
  int is_model=len>=12&&!memcmp(raw,"PIFF",4)&&!memcmp(raw+8,"3DOB",4);
  free(raw);
  if(!is_model)continue;
  cnk_material_reference names[512]={{0}};unsigned num=0;
  int res=cnk_model_material_from_vfs(&vfs,i,names,512,&num);
  if(i==70){
   assert(res!=0); /* selector #3 has no declared filename: strict API rejects. */
   assert(num==0);blocked++;continue;
  }
  assert(res==0);assert(num>0);
  for(unsigned j=0;j<num;j++){
   if(names[j].original_selector){named++;assert(strstr(names[j].source_png,".png")!=NULL);}
   else {untextured++;assert(names[j].source_png[0]==0);}
   groups++;
  }
 }
 assert(blocked==1);
 /* Model 70 has 2 verified names, but material selector #3 has no name.
  * Inspect it without guessing; strict renderer must still reject it. */
 unsigned char *special_data=NULL;size_t special_size=0;
 assert(!fufs_extract(&vfs,70,&special_data,&special_size));
 cnk_model plant_mesh={0};cnk_model_texl plant_names={0};
 assert(!cnk_parse_3dob(special_data,special_size,&plant_mesh));
 assert(!cnk_model_texl_parse(special_data,special_size,&plant_names));
 assert(plant_names.opaque_tail_bytes==18);
 cnk_material_reference inspected[512]={{0}};
 unsigned missing=100;
 assert(!cnk_model_material_inspect(&plant_mesh,&plant_names,inspected,512,&missing));
 assert(missing==1 && plant_mesh.num_groups==3);
 cnk_material_reference strict[3]={0};
 strcpy(strict[0].source_png,"keep-me");
 assert(cnk_model_material_resolve(&plant_mesh,&plant_names,strict,3)<0);
 assert(!strcmp(strict[0].source_png,"keep-me"));

 assert(inspected[0].original_selector==3 && !inspected[0].source_png[0]);
 assert(inspected[1].original_selector==1 && !strcmp(inspected[1].source_png,"plant3.png"));
 assert(inspected[2].original_selector==2 && !strcmp(inspected[2].source_png,"fleshplant.png"));
 cnk_model_free(&plant_mesh);free(special_data);
 assert(blocked==1);
 assert(named>30 && untextured==3);
 for(int f=0;f<6;f++)for(int k=0;k<3;k++){
  cnk_material_reference refs[512]={{0}};unsigned ng=0;
  assert(cnk_model_material_from_vfs(&vfs,families[f].idx[k],refs,512,&ng)==0);
  assert(ng>=1);
  int found=0;
  for(unsigned g=0;g<ng;g++)if(!strcmp(refs[g].source_png,families[f].name))found++;
  assert(found>0);
 }
 cnk_material_reference ref={0};
 cnk_triangle_group g={0};cnk_model fake={0};cnk_model_texl t={0};
 fake.num_groups=1;fake.groups=&g;t.count=1;strcpy(t.names[0],"real.png");
 strcpy(ref.source_png,"untouched");
 g.material=2;assert(cnk_model_material_resolve(&fake,&t,&ref,1)!=0);
 assert(!strcmp(ref.source_png,"untouched"));
 g.material=1;assert(cnk_model_material_resolve(&fake,&t,&ref,1)==0);
 assert(!strcmp(ref.source_png,"real.png"));
 g.material=0;assert(cnk_model_material_resolve(&fake,&t,&ref,1)==0);
 assert(ref.source_png[0]==0);
 fufs_close(&vfs);
 printf("model_materials: %u groups checked, named=%u, untextured=%u, special_metadata=%u, 18 LOD matches\n",groups,named,untextured,blocked);
 return 0;
}
