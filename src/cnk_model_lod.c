#include "cnk_model_lod.h"
#include <math.h>
#include <string.h>

void cnk_model_lod_free(cnk_model_lod *set){
 if(!set)return;
 for(unsigned i=0;i<3;i++)cnk_object_mesh_free(&set->levels[i]);
 memset(set,0,sizeof(*set));
}

static int plausible_variant(const cnk_object_mesh *a,const cnk_object_mesh *b){
 /* Reject clearly unrelated shapes. This is NOT proof of historic identity. */
 for(unsigned axis=0;axis<3;axis++){
  float lo=a->bbox_min[axis],hi=a->bbox_max[axis];
  float other_lo=b->bbox_min[axis],other_hi=b->bbox_max[axis];
  float d=hi-lo,other_d=other_hi-other_lo;
  if(!isfinite(d)||!isfinite(other_d)||d<=0||other_d<=0)return 0;
  if(fabsf(other_d-d)>fmaxf(0.60f*d,0.10f))return 0;
  float middle=(hi+lo)*0.5f,other_middle=(other_hi+other_lo)*0.5f;
  if(fabsf(other_middle-middle)>fmaxf(0.50f*d,0.12f))return 0;
 }
 return 1;
}
int cnk_model_lod_load(const fufs_file *vfs, const cnk_lod_source source[3],
                       cnk_model_lod *out){
 if(!vfs||!vfs->data||!vfs->entries||!source||!out)return -1;
 memset(out,0,sizeof(*out));
 cnk_model_lod stage={0};
 cnk_model_texl texl[3]={0};
 for(unsigned i=0;i<3;i++){
  if(source[i].vfs_index>=vfs->count||
     vfs->entries[source[i].vfs_index].id!=source[i].source_id){
    cnk_model_lod_free(&stage);return -2;
  }
  for(unsigned j=0;j<i;j++)if(source[i].vfs_index==source[j].vfs_index){
    cnk_model_lod_free(&stage);return -3;
  }
  if(cnk_object_mesh_from_vfs(vfs,source[i].vfs_index,&stage.levels[i])){
   cnk_model_lod_free(&stage);return -4;
  }
  stage.source[i]=source[i];
  if(cnk_model_texl_from_vfs(vfs,source[i].vfs_index,&texl[i])){
   cnk_model_lod_free(&stage);return -6;
  }
  if(i && (stage.levels[i-1].triangle_count<=stage.levels[i].triangle_count||
       !plausible_variant(&stage.levels[0],&stage.levels[i]))){
   cnk_model_lod_free(&stage);return -5;
  }
 }
 if(cnk_model_texl_shared_png(texl,stage.shared_texture_name)){
  cnk_model_lod_free(&stage);return -7;
 }
 *out=stage;return 0;
}
const cnk_object_mesh *cnk_model_lod_choose(const cnk_model_lod *set,
                             float d,float near_switch,float far_switch){
 if(!set||!set->levels[0].vertices||!set->levels[1].vertices||
    !set->levels[2].vertices||!isfinite(d)||!isfinite(near_switch)||
    !isfinite(far_switch)||d<0||near_switch<0||near_switch>=far_switch)return NULL;
 return &set->levels[d<near_switch?0:(d<far_switch?1:2)];
}
