#include "cnk_nav.h"
#include <math.h>
#include <float.h>
static cnk_vec3 diff(cnk_vec3 a,cnk_vec3 b){cnk_vec3 v={a.x-b.x,a.y-b.y,a.z-b.z};return v;}
static float dot(cnk_vec3 a,cnk_vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static float dist(cnk_vec3 a,cnk_vec3 b){cnk_vec3 v=diff(a,b);return sqrtf(dot(v,v));}
static cnk_vec3 blend(cnk_vec3 a,cnk_vec3 b,float t){cnk_vec3 v={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};return v;}
float cnk_nav_length(const cnk_sline *line,int closed){
 if(!line || !line->points || line->count<2)return 0;
 uint32_t n=closed?line->count:line->count-1;
 float total=0;
 for(uint32_t i=0;i<n;i++)total+=dist(line->points[i],line->points[(i+1)%line->count]);
 return total;
}
int cnk_nav_sample_with_length(const cnk_sline *line,float fraction,int closed,
                               float total,cnk_vec3 *result){
 if(!line || !result || line->count<2 || !line->points || !isfinite(fraction) ||
    !isfinite(total))return -1;
 if(total<=0)return -2;
 if(closed){fraction-=floorf(fraction);}else{if(fraction<0)fraction=0;if(fraction>1)fraction=1;}
 float target=total*fraction;
 uint32_t n=closed?line->count:line->count-1;
 for(uint32_t i=0;i<n;i++){
  cnk_vec3 a=line->points[i],b=line->points[(i+1)%line->count];
  float len=dist(a,b);
  if(i==n-1 || target<=len){*result=blend(a,b,len>0?target/len:0);return 0;}
  target-=len;
 }
 return -3;
}
int cnk_nav_sample(const cnk_sline *line,float fraction,int closed,cnk_vec3 *result){
 if(!line || !result || line->count<2 || !line->points || !isfinite(fraction))return -1;
 return cnk_nav_sample_with_length(line,fraction,closed,cnk_nav_length(line,closed),result);
}
int cnk_nav_nearest(const cnk_sline *line,cnk_vec3 position,int closed,cnk_nav_projection *result){
 if(!line || !result || line->count<2 || !line->points)return -1;
 uint32_t n=closed?line->count:line->count-1;
 float best=FLT_MAX, travelled=0, best_length=0, total=cnk_nav_length(line,closed);
 if(total<=0)return -2;
 for(uint32_t i=0;i<n;i++){
  cnk_vec3 a=line->points[i],b=line->points[(i+1)%line->count],ab=diff(b,a);
  float len2=dot(ab,ab),t=len2>0?dot(diff(position,a),ab)/len2:0;
  if(t<0)t=0;
  if(t>1)t=1;
  cnk_vec3 projected=blend(a,b,t);float d=dist(position,projected);
  if(d<best){best=d;result->point=projected;result->segment=i;best_length=travelled+t*sqrtf(len2);}
  travelled+=sqrtf(len2);
 }
 result->distance=best;
 result->fraction=best_length/total;
 if(result->fraction>1)result->fraction=1;
 return 0;
}
static float side(cnk_vec3 a,cnk_vec3 b,cnk_vec3 p){return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x);}
int cnk_nav_gate_crossing(const cnk_sline *lap,cnk_vec3 before,cnk_vec3 after){
 if(!lap || lap->count!=2 || !lap->points)return 0;
 cnk_vec3 a=lap->points[0],b=lap->points[1];
 float s0=side(a,b,before),s1=side(a,b,after);
 if(s0==0 || s1==0 || (s0>0)==(s1>0))return 0;
 /* The crossed segment must intersect the finite LAP line, not its extension. */
 float t=s0/(s0-s1);cnk_vec3 c=blend(before,after,t);
 float dx=b.x-a.x,dz=b.z-a.z,den=dx*dx+dz*dz;
 if(den<=0)return 0;
 float u=((c.x-a.x)*dx+(c.z-a.z)*dz)/den;
 return (u>=0 && u<=1)? (s0<0?1:-1):0;
}
