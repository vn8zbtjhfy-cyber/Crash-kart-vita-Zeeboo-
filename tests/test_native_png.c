#include "cnk_png.h"
#include "cnk_scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int synthetic(const cnk_png_catalog *cat){
 unsigned char png[96]={0};cnk_png_info info={0};
 if(cnk_png_inspect(NULL,0,&info)!=-1)return 1;
 if(cnk_png_inspect((const unsigned char*)"abc",3,&info)!=-2)return 2;
 if(!cat->count)return 3;
 uint32_t known=cat->items[0].vfs_index;
 const cnk_png_resource *r=cnk_png_find_index(cat,known);
 if(!r||r->vfs_index!=known)return 4;
 cnk_texture_binding valid[1]={{0,known}};
 if(cnk_png_binding_check(cat,1,valid,1))return 5;
 cnk_texture_binding bad_index[1]={{1,known}};
 if(cnk_png_binding_check(cat,1,bad_index,1)!=-2)return 6;
 cnk_texture_binding double_map[2]={{0,known},{0,known}};
 if(cnk_png_binding_check(cat,2,double_map,2)!=-3)return 7;
 memcpy(png,"\x89PNG\r\n\x1a\n",8);
 if(cnk_png_inspect(png,sizeof(png),&info)==0)return 8;
 return 0;
}
int main(int argc,char **argv){
 if(argc!=2)return 1;
 fufs_file f={0};cnk_png_catalog cat={0};
 if(fufs_open(argv[1],&f)||cnk_png_catalog_build(&f,&cat))return 2;
 int rc=synthetic(&cat);
 if(rc){fprintf(stderr,"Synthetic PNG tests fail %d\n",rc);return 3;}
 uint32_t max_width=0,max_height=0,scenes=0,texture_refs=0,unnamed=0;
 for(size_t i=0;i<cat.count;i++){
  unsigned char *data=NULL;size_t n=0;
  cnk_png_info info={0};
  const cnk_png_resource *r=cat.items+i;
  if(fufs_extract(&f,r->vfs_index,&data,&n)||cnk_png_inspect(data,n,&info))return 4;
  if(memcmp(&info,&r->image,sizeof(info)))return 5;
  free(data);
  if(info.width>max_width)max_width=info.width;
  if(info.height>max_height)max_height=info.height;
 }
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *data=NULL;size_t n=0;
  if(fufs_extract(&f,i,&data,&n))return 6;
  if(n>=12&&!memcmp(data+8,"SCEN",4)){
   cnk_scene s={0};if(cnk_parse_scene(data,n,&s))return 7;
   scenes++;texture_refs+=s.texture_count;
   for(uint32_t j=0;j<s.texture_count;j++)if(!s.texture_names[j][0])unnamed++;
   /* The PNG catalog alone never invents a name -> index mapping. */
   if(cnk_png_binding_check(&cat,s.texture_count,NULL,0))return 8;
   cnk_scene_free(&s);
  }
  free(data);
 }
 printf("PNG verified=%zu, scenes=%u, TXTR references=%u, unnamed=%u, max dimensions=%ux%u\n",
  cat.count,scenes,texture_refs,unnamed,max_width,max_height);
 rc=(cat.count==317&&scenes==14&&texture_refs==425)?0:9;
 cnk_png_catalog_free(&cat);fufs_close(&f);
 if(rc)return rc;
 puts("PASS: Original PNG CRC and dimensions verified. No unproven name-resource matches.");
 return 0;
}
