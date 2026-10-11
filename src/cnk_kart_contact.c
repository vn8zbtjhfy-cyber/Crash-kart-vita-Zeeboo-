#include "cnk_kart_contact.h"
#include <math.h>
#include <string.h>

static int finite3(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static float clamp01(float t){return t<0?0:t>1?1:t;}
void cnk_kart_contact_default(cnk_kart_contact_tuning *out){
 if(out)*out=(cnk_kart_contact_tuning){0.15f,1.25f};
}
int cnk_kart_contact_validate(const cnk_kart_contact_tuning *c){
 return (!c||!isfinite(c->radius)||!isfinite(c->height_tolerance)||
  c->radius<=0.0f||c->radius>20.0f||
  c->height_tolerance<=0.0f||c->height_tolerance>100.0f)?-1:0;
}
static void direction(float dx,float dz,float *nx,float *nz){
 float len=sqrtf(dx*dx+dz*dz);
 if(len<0.000001f){*nx=1.0f;*nz=0.0f;}
 else {*nx=dx/len;*nz=dz/len;}
}
/* Collision of two moving horizontal discs via their relative segment. */
static int swept_pair(cnk_vec3 a,cnk_vec3 a_end,cnk_vec3 b,cnk_vec3 b_end,
                      float diameter,float vertical_tolerance,float *out_t){
 float px=a.x-b.x,pz=a.z-b.z;
 float vx=(a_end.x-a.x)-(b_end.x-b.x);
 float vz=(a_end.z-a.z)-(b_end.z-b.z);
 float c=px*px+pz*pz-diameter*diameter;
 float t=0;
 if(c>0){
  float qa=vx*vx+vz*vz;
  float qb=2*(px*vx+pz*vz);
  float disc=qb*qb-4*qa*c;
  if(qa<1.e-12f||qb>=0||disc<0)return 0;
  t=(-qb-sqrtf(fmaxf(0,disc)))/(2*qa);
  if(t<0||t>1)return 0;
 }else{
  /* Already touching: ignore separation if endpoints cease overlapping. */
  float ex=a_end.x-b_end.x,ez=a_end.z-b_end.z;
  if(ex*ex+ez*ez>=diameter*diameter && px*vx+pz*vz>=0)return 0;
 }
 float ay=a.y+(a_end.y-a.y)*t;
 float by=b.y+(b_end.y-b.y)*t;
 if(fabsf(ay-by)>vertical_tolerance)return 0;
 *out_t=clamp01(t);return 1;
}
int cnk_kart_contacts_resolve(cnk_kart *karts,const cnk_vec3 *before,
  const uint8_t *movable,uint32_t count,
  const cnk_kart_contact_tuning *cfg,uint8_t *contact_flags){
 if(!karts||!before||!movable||!contact_flags||count<1||
    count>CNK_KART_CONTACT_MAX||cnk_kart_contact_validate(cfg))return -1;
 for(uint32_t i=0;i<count;i++){
  if(!finite3(before[i])||!finite3(karts[i].position)||
   !isfinite(karts[i].speed)||!isfinite(karts[i].heading)||
   !isfinite(karts[i].steering)||movable[i]>1)return -2;
 }
 cnk_kart next[CNK_KART_CONTACT_MAX];
 uint8_t touched[CNK_KART_CONTACT_MAX]={0};
 uint8_t pair_seen[CNK_KART_CONTACT_MAX][CNK_KART_CONTACT_MAX]={{0}};
 /* A runner stopped by an early impact must not also collide with an
  * unrelated racer farther along its *untraveled* predicted path. */
 typedef struct {uint32_t a,b;float time;} cnk_contact_candidate;
 cnk_contact_candidate candidates[CNK_KART_CONTACT_MAX*(CNK_KART_CONTACT_MAX-1)/2];
 uint32_t candidate_count=0;
 uint8_t stopped[CNK_KART_CONTACT_MAX]={0};
 memcpy(next,karts,sizeof(cnk_kart)*count);
 int pair_count=0;
 float diameter=cfg->radius*2.0f;
 for(uint32_t i=0;i<count;i++)for(uint32_t j=i+1;j<count;j++){
  float t;
  if(!movable[i]&&!movable[j])continue;
  if(!swept_pair(before[i],karts[i].position,before[j],karts[j].position,
                 diameter,cfg->height_tolerance,&t))continue;
  candidates[candidate_count++]=(cnk_contact_candidate){i,j,t};
 }
 /* Process swept contacts chronologically, not in racer slot order. A later
  * predicted crossing is discarded if one of its participants has already
  * stopped earlier in this frame. This is still provisional disc physics. */
 for(uint32_t a=1;a<candidate_count;a++){
  cnk_contact_candidate c=candidates[a];uint32_t b=a;
  while(b>0&&candidates[b-1].time>c.time){
   candidates[b]=candidates[b-1];b--;
  }
  candidates[b]=c;
 }
 for(uint32_t n=0;n<candidate_count;n++){
  cnk_contact_candidate c=candidates[n];
  uint32_t i=c.a,j=c.b;
  if(stopped[i]||stopped[j])continue;
  touched[i]=touched[j]=1;pair_seen[i][j]=1;pair_count++;
  const float safe=fmaxf(0.0f,c.time-0.0005f);
  if(movable[i]){
   next[i].position.x=before[i].x+(karts[i].position.x-before[i].x)*safe;
   next[i].position.y=before[i].y+(karts[i].position.y-before[i].y)*safe;
   next[i].position.z=before[i].z+(karts[i].position.z-before[i].z)*safe;
   next[i].speed=0;
   stopped[i]=1;
  }
  if(movable[j]){
   next[j].position.x=before[j].x+(karts[j].position.x-before[j].x)*safe;
   next[j].position.y=before[j].y+(karts[j].position.y-before[j].y)*safe;
   next[j].position.z=before[j].z+(karts[j].position.z-before[j].z)*safe;
   next[j].speed=0;
   stopped[j]=1;
  }
 }
 /* Separate initially overlapping discs and chains after first contacts.
  * No dynamic impulse, skid, stun, attack or tyre physics is asserted. */
 for(int pass=0;pass<8;pass++){
  int resolved=0;
  for(uint32_t i=0;i<count;i++)for(uint32_t j=i+1;j<count;j++){
   if(!movable[i]&&!movable[j])continue;
   cnk_vec3 a=next[i].position,b=next[j].position;
   if(fabsf(a.y-b.y)>cfg->height_tolerance)continue;
   float dx=a.x-b.x,dz=a.z-b.z,dist=sqrtf(dx*dx+dz*dz);
   float overlap=diameter-dist;
   if(overlap<=0.00001f)continue;
   float nx,nz;direction(dx,dz,&nx,&nz);
   float each=overlap/(float)(movable[i]+movable[j]);
   if(movable[i]){next[i].position.x+=nx*each;next[i].position.z+=nz*each;next[i].speed=0;}
   if(movable[j]){next[j].position.x-=nx*each;next[j].position.z-=nz*each;next[j].speed=0;}
   if(!pair_seen[i][j]){pair_count++;pair_seen[i][j]=1;}
   touched[i]=touched[j]=1;
   resolved=1;
  }
  if(!resolved)break;
 }
 for(uint32_t i=0;i<count;i++){
  if(!finite3(next[i].position)||!isfinite(next[i].speed))return -3;
 }
 memcpy(karts,next,sizeof(cnk_kart)*count);
 memcpy(contact_flags,touched,count);
 return pair_count;
}
