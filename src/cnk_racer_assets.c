#include "cnk_racer_assets.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int dec32(const char *s,uint32_t *out){
 uint32_t n=0;
 if(!s||!*s)return -1;
 for(;*s;s++){
  if(*s<'0'||*s>'9')return -1;
  unsigned v=(unsigned)(*s-'0');
  if(n>(UINT32_MAX-v)/10u)return -1;
  n=n*10u+v;
 }
 *out=n;return 0;
}
static int hex32(const char *s,uint32_t *out){
 if(!s||strlen(s)!=8)return -1;
 uint32_t n=0;
 for(;*s;s++){
  unsigned char c=(unsigned char)*s;
  unsigned v;
  if(c>='0'&&c<='9')v=(unsigned)(c-'0');
  else if(c>='a'&&c<='f')v=(unsigned)(c-'a'+10);
  else if(c>='A'&&c<='F')v=(unsigned)(c-'A'+10);
  else return -1;
  n=(n<<4)|v;
 }
 *out=n;return 0;
}
static int finite_float(const char *s,float *out){
 char *end=NULL;
 if(!s||!*s)return -1;
 errno=0;
 float f=strtof(s,&end);
 if(errno==ERANGE||!end||*end||!isfinite(f))return -1;
 *out=f;
 return 0;
}
static int parse_matrix(char *const *fields,unsigned offset,float m[16]){
 for(unsigned i=0;i<16;i++)if(finite_float(fields[i+offset],&m[i]))return -2;
 /* Column-major affine matrix with invertible linear part. */
 if(fabsf(m[3])>1e-6f||fabsf(m[7])>1e-6f||
    fabsf(m[11])>1e-6f||fabsf(m[15]-1)>1e-6f)return -3;
 float det=m[0]*(m[5]*m[10]-m[9]*m[6])-
           m[4]*(m[1]*m[10]-m[9]*m[2])+
           m[8]*(m[1]*m[6]-m[5]*m[2]);
 if(!isfinite(det)||fabsf(det)<1e-8f)return -3;
 return 0;
}
void cnk_racer_assets_free(cnk_racer_assets *a){
 if(!a)return;
 for(unsigned i=0;i<CNK_MAX_RACERS;i++){
  cnk_object_mesh_free(&a->meshes[i]);
  cnk_model_lod_free(&a->lods[i]);
 }
 memset(a,0,sizeof(*a));
}
int cnk_racer_assets_load(const char *path,const fufs_file *vfs,cnk_racer_assets *out){
 if(!out||!path||!*path||!vfs||!vfs->data||!vfs->entries)return -1;
 memset(out,0,sizeof(*out));
 FILE *fp=fopen(path,"rb");
 if(!fp)return errno==ENOENT?-2:-3;
 cnk_racer_assets stage={0};
 char line[2048];int err=0;
 while(fgets(line,sizeof(line),fp)){
  size_t n=strlen(line);
  if(n==sizeof(line)-1&&line[n-1]!='\n'){err=-4;break;}
  if(n&&line[n-1]=='\n')line[--n]=0;
  if(n&&line[n-1]=='\r')line[--n]=0;
  if(!n||line[0]=='#')continue;
  char *fields[25];unsigned nf=1;fields[0]=line;
  for(char *p=line;*p;p++)if(*p=='\t'){
   if(nf==25){err=-5;break;}
   *p=0;fields[nf++]=p+1;
  }
  if(err)break;
  uint32_t slot=0;
  if((nf!=19&&nf!=25)||dec32(fields[0],&slot)||slot>=CNK_MAX_RACERS||
     stage.assigned[slot]){err=-5;break;}
  float matrix[16];
  int has_lod=nf==25;
  if(parse_matrix(fields,has_lod?9u:3u,matrix)){err=-5;break;}
  if(has_lod){
   cnk_lod_source refs[3];float near_d=0,far_d=0;
   for(unsigned i=0;i<3;i++){
    if(dec32(fields[1+i*2],&refs[i].vfs_index)||
       hex32(fields[2+i*2],&refs[i].source_id)){err=-5;break;}
   }
   if(err)break;
   if(finite_float(fields[7],&near_d)||finite_float(fields[8],&far_d)||
      near_d<0||far_d<=near_d){err=-5;break;}
   if(cnk_model_lod_load(vfs,refs,&stage.lods[slot])){err=-7;break;}
   stage.has_lod[slot]=1;
   stage.switch_near[slot]=near_d;
   stage.switch_far[slot]=far_d;
   stage.index[slot]=refs[0].vfs_index;
   stage.resource_id[slot]=refs[0].source_id;
  }else{
   uint32_t index=0,id=0;
   if(dec32(fields[1],&index)||hex32(fields[2],&id)||index>=vfs->count||
      vfs->entries[index].id!=id){err=-6;break;}
   if(cnk_object_mesh_from_vfs(vfs,index,&stage.meshes[slot])){err=-7;break;}
   stage.index[slot]=index;stage.resource_id[slot]=id;
  }
  memcpy(stage.calibration[slot],matrix,sizeof(matrix));
  stage.assigned[slot]=1;
 }
 if(ferror(fp))err=-8;
 fclose(fp);
 if(err){cnk_racer_assets_free(&stage);return err;}
 *out=stage;return 0;
}
void cnk_racer_assets_attachments(const cnk_racer_assets *assets,
                  cnk_racer_visual_attachment out[CNK_MAX_RACERS]){
 if(!out)return;
 memset(out,0,sizeof(cnk_racer_visual_attachment)*CNK_MAX_RACERS);
 if(!assets)return;
 for(unsigned i=0;i<CNK_MAX_RACERS;i++)if(assets->assigned[i]){
  out[i].mesh=assets->has_lod[i]?&assets->lods[i].levels[0]:&assets->meshes[i];
  out[i].local_from_mesh=assets->calibration[i];
 }
}
int cnk_racer_assets_attachments_for_camera(const cnk_racer_assets *assets,
       const cnk_session *session,cnk_racer_visual_attachment out[CNK_MAX_RACERS]){
 if(!assets||!session||!out||!session->active||
    !session->racer_count||session->racer_count>CNK_MAX_RACERS)return -1;
 cnk_racer_visual_attachment stage[CNK_MAX_RACERS]={{0}};
 cnk_vec3 eye=session->camera.eye;
 if(!isfinite(eye.x)||!isfinite(eye.y)||!isfinite(eye.z))return -2;
 for(unsigned i=0;i<session->racer_count;i++)if(assets->assigned[i]){
  stage[i].local_from_mesh=assets->calibration[i];
  if(assets->has_lod[i]){
   cnk_vec3 pos=session->racers[i].kart.position;
   if(!isfinite(pos.x)||!isfinite(pos.y)||!isfinite(pos.z))return -3;
   /* double arithmetic avoids overflow while subtracting large positions. */
   double dx=(double)eye.x-pos.x,dy=(double)eye.y-pos.y,dz=(double)eye.z-pos.z;
   double distance=hypot(hypot(dx,dy),dz);
   if(!isfinite(distance))return -3;
   /* Distances larger than FLT_MAX must choose the far LOD, not wrap. */
   if(distance>=assets->switch_far[i])stage[i].mesh=&assets->lods[i].levels[2];
   else stage[i].mesh=cnk_model_lod_choose(&assets->lods[i],(float)distance,
                     assets->switch_near[i],assets->switch_far[i]);
   if(!stage[i].mesh)return -4;
  }else stage[i].mesh=&assets->meshes[i];
 }
 memcpy(out,stage,sizeof(stage));
 return 0;
}
