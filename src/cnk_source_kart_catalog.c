#include "cnk_source_kart_catalog.h"
/* All indices/IDs match the FUFS table and the original LOD material audit. */
static const struct {uint32_t index,id;} models[6][3]={
 {{5,0x02d15519},{8,0x0404d06a},{12,0x05384bbb}},
 {{26,0x0e743c14},{29,0x0fa7b765},{30,0x10db32b6}},
 {{40,0x17972f64},{43,0x18caaab5},{45,0x19fe2606}},
 {{173,0x6d7771e0},{174,0x6eaaed31},{178,0x6fde6882}},
 {{249,0x9f4ca92e},{254,0xa080247f},{257,0xa1b39fd0}},
 {{395,0xf95f2dec},{399,0xfa92a93d},{401,0xfbc6248e}}
};
int cnk_source_kart_gallery_source(unsigned family,unsigned lod,
                                   uint32_t *index,uint32_t *id){
 if(family>=6||lod>=3||!index||!id)return -1;
 *index=models[family][lod].index;
 *id=models[family][lod].id;
 return 0;
}
