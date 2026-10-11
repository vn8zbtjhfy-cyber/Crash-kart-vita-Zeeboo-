#include "cnk_model_lod.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static const unsigned candidates[6][3]={{5,8,12},{26,29,30},{40,43,45},
                                        {173,174,178},{249,254,257},{395,399,401}};
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_model_lod model={0};
 for(unsigned family=0;family<6;family++){
  cnk_lod_source refs[3];
  for(unsigned i=0;i<3;i++){
   refs[i].vfs_index=candidates[family][i];
   refs[i].source_id=vfs.entries[refs[i].vfs_index].id;
  }
  assert(!cnk_model_lod_load(&vfs,refs,&model));
  assert(model.levels[0].triangle_count>model.levels[1].triangle_count);
  assert(model.levels[1].triangle_count>model.levels[2].triangle_count);
  assert(cnk_model_lod_choose(&model,2,5,15)==&model.levels[0]);
  assert(cnk_model_lod_choose(&model,5,5,15)==&model.levels[1]);
  assert(cnk_model_lod_choose(&model,15,5,15)==&model.levels[2]);
  assert(!cnk_model_lod_choose(&model,-1,5,15));
  assert(!cnk_model_lod_choose(&model,NAN,5,15));
  assert(!cnk_model_lod_choose(&model,2,15,5));
  assert(!cnk_model_lod_choose(&model,2,5,INFINITY));
  cnk_model_lod_free(&model);
  assert(!model.levels[0].vertices);
  /* Wrong resource ID, repeated slot and reversed detail must roll back. */
  refs[1].source_id^=1u;
  assert(cnk_model_lod_load(&vfs,refs,&model)<0);
  assert(!model.levels[0].vertices);
  refs[1].source_id^=1u;refs[1]=refs[0];
  assert(cnk_model_lod_load(&vfs,refs,&model)<0);
  assert(!model.levels[0].vertices);
  refs[1].vfs_index=candidates[family][2];
  refs[1].source_id=vfs.entries[refs[1].vfs_index].id;
  refs[2].vfs_index=candidates[family][1];
  refs[2].source_id=vfs.entries[refs[2].vfs_index].id;
  assert(cnk_model_lod_load(&vfs,refs,&model)<0);
  assert(!model.levels[0].vertices);
 }
 fufs_close(&vfs);
 puts("PASS: six source-consistent high/medium/low 3DOB candidate families; no racer identities or original LOD distances inferred");
 return 0;
}
