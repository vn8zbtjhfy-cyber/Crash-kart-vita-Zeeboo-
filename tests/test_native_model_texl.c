#include "cnk_model_texl.h"
#include "cnk_model_lod.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned slots[6][3]={{5,8,12},{26,29,30},{40,43,45},
                                    {173,174,178},{249,254,257},{395,399,401}};
static const char *labels[6]={"NeoSprint01.png","LeMans02.png","cocster2.png",
                              "rallye.png","CrashStang128_t.png","NeoMini.png"};
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 for(unsigned family=0;family<6;family++){
  cnk_lod_source refs[3];cnk_model_texl lists[3]={0};
  for(unsigned level=0;level<3;level++){
   unsigned idx=slots[family][level];
   refs[level]=(cnk_lod_source){idx,vfs.entries[idx].id};
   assert(!cnk_model_texl_from_vfs(&vfs,idx,&lists[level]));
   assert(lists[level].count>0);
  }
  char label[CNK_TEXL_NAME_CAP]={0};
  assert(!cnk_model_texl_shared_png(lists,label));
  assert(!strcmp(label,labels[family]));
  cnk_model_lod set={0};
  assert(!cnk_model_lod_load(&vfs,refs,&set));
  assert(!strcmp(set.shared_texture_name,labels[family]));
  cnk_model_lod_free(&set);
  assert(!set.shared_texture_name[0]);
  assert(!set.levels[0].vertices);
  printf("family=%u model=%s slots=%u,%u,%u\n",family+1,label,
         slots[family][0],slots[family][1],slots[family][2]);
 }
 /* Source #70: two confirmed TEXL names and an uninterpreted binary suffix. */
 cnk_model_texl plant={0};
 assert(!cnk_model_texl_from_vfs(&vfs,70,&plant));
 assert(plant.count==2 && plant.opaque_tail_bytes==18);
 assert(!strcmp(plant.names[0],"plant3.png"));
 assert(!strcmp(plant.names[1],"fleshplant.png"));
 /* Mutating the opaque record must reject it, not silently turn it into a name. */
 unsigned char *plantbytes=NULL;size_t plantlen=0;
 assert(!fufs_extract(&vfs,70,&plantbytes,&plantlen));
 assert(plantlen>44);
 /* Only the exact observed trailer is accepted. Corruption is rejected and
  * the caller's result stays clean instead of keeping guessed names. */
 size_t texpos=0;
 for(size_t off=12;off+8<=plantlen;){
  size_t size=(size_t)plantbytes[off+4]|
    ((size_t)plantbytes[off+5]<<8)|((size_t)plantbytes[off+6]<<16)|
    ((size_t)plantbytes[off+7]<<24);
  if(!memcmp(plantbytes+off,"TEXL",4)){texpos=off;break;}
  assert(size<=plantlen-off-8);off+=8+size;
 }
 assert(texpos>0);
 size_t tailbyte=texpos+8+43;
 unsigned char remembered=plantbytes[tailbyte];plantbytes[tailbyte]^=1;
 cnk_model_texl bad={0};
 assert(cnk_model_texl_parse(plantbytes,plantlen,&bad)<0);
 assert(bad.count==0 && bad.opaque_tail_bytes==0);
 plantbytes[tailbyte]=remembered;
 assert(!cnk_model_texl_parse(plantbytes,plantlen,&bad));
 assert(bad.opaque_tail_bytes==18 && bad.count==2);
 free(plantbytes);
 /* Cross-livery variants must not be given a shared texture by guessing. */
 cnk_model_texl mixed[3]={0};
 assert(!cnk_model_texl_from_vfs(&vfs,5,&mixed[0]));
 assert(!cnk_model_texl_from_vfs(&vfs,29,&mixed[1]));
 assert(!cnk_model_texl_from_vfs(&vfs,12,&mixed[2]));
 char shared[CNK_TEXL_NAME_CAP]={0};
 assert(cnk_model_texl_shared_png(mixed,shared)<0);
 assert(!shared[0]);
 /* Malformed PIFF: size and truncated TEXL are never trusted. */
 cnk_model_texl result={0};
 unsigned char invalid[]={ 'P','I','F','F',0x16,0,0,0,
                           '3','D','O','B','T','E','X','L',0x02,0,0,0,'x','y' };
 assert(cnk_model_texl_parse(invalid,sizeof(invalid),&result)<0);
 assert(!result.count);
 unsigned char *bytes=NULL;size_t len=0;
 assert(!fufs_extract(&vfs,5,&bytes,&len));
 assert(len>12);
 unsigned char saved=bytes[4];bytes[4]^=1;
 assert(cnk_model_texl_parse(bytes,len,&result)<0);
 bytes[4]=saved;
 assert(!cnk_model_texl_parse(bytes,len,&result));
 free(bytes);
 fufs_close(&vfs);
 puts("PASS: six real vehicle LOD families share explicit TEXL names in every resolution; PNG VFS IDs and drivers not inferred");
 return 0;
}
