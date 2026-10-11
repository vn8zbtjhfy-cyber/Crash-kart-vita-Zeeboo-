#include "cnk_race.h"
#include "cnk_nav.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

static float side(cnk_vec3 a,cnk_vec3 b,cnk_vec3 p){
 return (b.x-a.x)*(p.z-a.z)-(b.z-a.z)*(p.x-a.x);
}
static float segment_length(cnk_vec3 a,cnk_vec3 b){
 float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z;
 return sqrtf(dx*dx+dy*dy+dz*dz);
}
static float fraction_cross(cnk_vec3 a,cnk_vec3 b,
                            cnk_vec3 before,cnk_vec3 after){
 float u=side(a,b,before),v=side(a,b,after);
 return u/(u-v);
}
static int by_fraction(const void *a,const void *b){
 const cnk_race_gate *x=(const cnk_race_gate*)a,*y=(const cnk_race_gate*)b;
 return (x->fraction>y->fraction)-(x->fraction<y->fraction);
}
int cnk_race_track_build(const cnk_scene *s,cnk_race_track *out){
 const cnk_sline *drive;
 cnk_race_track tmp;
 float length, traveled=0;
 int finish=-1;
 if(!out)return -1;
 memset(out,0,sizeof(*out));memset(&tmp,0,sizeof(tmp));
 if(!s || !(drive=cnk_scene_line(s,"DRV0")) || drive->count<3 ||
    !drive->points)return -3;
 length=cnk_nav_length(drive,1);
 if(!isfinite(length)||length<1.0f)return -4;
 for(uint32_t k=0;k<s->line_count;k++){
  const cnk_sline *l=&s->lines[k];
  if(strncmp(l->id,"LAP",3))continue;
  if(tmp.gate_count>=CNK_MAX_GATES || l->count!=2 || !l->points)return -4;
  cnk_race_gate *g=&tmp.gates[tmp.gate_count++];
  memcpy(g->id,l->id,5);g->a=l->points[0];g->b=l->points[1];
  if(segment_length(g->a,g->b)<0.01f)return -4;
 }
 if(tmp.gate_count<2)return -3;
 /* Reject additional route crossings and checkpoints that the source driving
  * route never crosses. In those cases a race would silently count incorrectly. */
 for(uint32_t k=0;k<tmp.gate_count;k++){
  cnk_race_gate *g=&tmp.gates[k];
  uint32_t hits=0;
  traveled=0;
  for(uint32_t j=0;j<drive->count;j++){
   cnk_vec3 a=drive->points[j],b=drive->points[(j+1)%drive->count];
   float d=segment_length(a,b);
   int dir;
   /* A DRV vertex may lie *exactly* on a finite LAP segment. In that
    * case the ordinary segment-crossing test omits both adjacent edges.
    * Inspect the two neighboring samples once at that vertex. */
   if(fabsf(side(g->a,g->b,a))<0.0001f){
    cnk_vec3 prev=drive->points[(j+drive->count-1)%drive->count];
    float before=side(g->a,g->b,prev),after=side(g->a,g->b,b);
    float dx=g->b.x-g->a.x,dz=g->b.z-g->a.z;
    float den=dx*dx+dz*dz;
    float u=den>0.00001f?((a.x-g->a.x)*dx+(a.z-g->a.z)*dz)/den:-1.0f;
    if(before*after<0.0f && u>=0.0f && u<=1.0f){
     if(hits++)return -4;
     g->direction=before<0.0f?1:-1;
     g->fraction=traveled/length;
     if(g->fraction>=1.0f)g->fraction=0.0f;
    }
   }
   /* The source gate points remain owned by the scene; use local view. */
   cnk_sline gate={0};gate.points=&g->a;gate.count=2;
   dir=(fabsf(side(g->a,g->b,a))<0.0001f ||
        fabsf(side(g->a,g->b,b))<0.0001f) ? 0 : cnk_nav_gate_crossing(&gate,a,b);
   if(dir){
    float u=fraction_cross(g->a,g->b,a,b);
    if(hits++)return -4;
    g->direction=dir;
    g->fraction=(traveled+u*d)/length;
    if(g->fraction>=1.0f)g->fraction=0.0f;
   }
   traveled+=d;
  }
  if(hits!=1)return -4;
 }
 qsort(tmp.gates,tmp.gate_count,sizeof(tmp.gates[0]),by_fraction);
 for(uint32_t k=0;k<tmp.gate_count;k++){
  if(!strcmp(tmp.gates[k].id,"LAP0")){finish=(int)k;break;}
 }
 if(finish<0)return -4;
 /* Rotate to checkpoint 0 = finish line and then checkpoints in driving order. */
 cnk_race_track result={0};result.gate_count=tmp.gate_count;
 for(uint32_t k=0;k<tmp.gate_count;k++)
  result.gates[k]=tmp.gates[(finish+k)%tmp.gate_count];
 *out=result;
 return 0;
}
int cnk_race_start(cnk_race *race,const cnk_race_track *track,
                   uint32_t laps,uint64_t now_ms){
 if(!race)return -1;
 memset(race,0,sizeof(*race));
 if(!track||track->gate_count<2||track->gate_count>CNK_MAX_GATES||!laps||laps>999)return -2;
 race->track=track;race->total_laps=laps;race->next_gate=1;
 race->last_accepted_gate=UINT32_MAX;
 race->start_ms=now_ms;race->lap_start_ms=now_ms;
 race->sector_start_ms=now_ms;race->last_sector_gate=UINT32_MAX;
 race->sector_valid=1;
 race->last_ms=now_ms;race->running=1;
 return 0;
}
int cnk_race_advance(cnk_race *race,cnk_vec3 before,cnk_vec3 after,uint64_t now_ms){
 if(!race||!race->track || !race->running)return -1;
 if(now_ms<race->last_ms)return -2;
 if(!isfinite(before.x)||!isfinite(before.z)||!isfinite(after.x)||!isfinite(after.z))return -3;
 uint64_t previous_ms=race->last_ms;
 race->last_ms=now_ms;
 /* A movement can intersect more than one gate. Process intersections in
  * geometric travel order, not array order, to avoid skipping checkpoints. */
 typedef struct {uint32_t index;float along;int dir;} hit;
 hit hits[CNK_MAX_GATES];uint32_t n=0;
 for(uint32_t k=0;k<race->track->gate_count;k++){
  const cnk_race_gate *g=&race->track->gates[k];
  cnk_vec3 endpoints[2]={g->a,g->b};
  cnk_sline line={0};line.count=2;line.points=endpoints;
  float a=side(g->a,g->b,before),b=side(g->a,g->b,after);
  /* Preserve the sign across a frame that ends exactly on the gate. */
  int old_sign=a>0.0001f?1:a< -0.0001f?-1:race->last_side[k];
  int new_sign=b>0.0001f?1:b< -0.0001f?-1:old_sign;
  int dir=0;float along=0;
  if(old_sign!=0 && new_sign!=0 && old_sign!=new_sign){
   if(fabsf(a)<0.0001f){
    /* Just left the checkpoint after landing exactly on its segment. */
    float dx=g->b.x-g->a.x,dz=g->b.z-g->a.z;
    float len2=dx*dx+dz*dz;
    float u=len2>0.00001f?((before.x-g->a.x)*dx+(before.z-g->a.z)*dz)/len2:-1;
    if(u>=0&&u<=1){dir=old_sign<0?1:-1;along=0.0f;}
   }else if(fabsf(b)>=0.0001f){
    dir=cnk_nav_gate_crossing(&line,before,after);
    if(dir)along=fraction_cross(g->a,g->b,before,after);
   }
  }
  race->last_side[k]=(signed char)new_sign;
  if(dir){
   hits[n].index=k;hits[n].along=along;
   hits[n++].dir=dir;
  }
 }
 for(uint32_t k=1;k<n;k++){
  hit x=hits[k];uint32_t j=k;
  while(j>0&&hits[j-1].along>x.along){hits[j]=hits[j-1];j--;}
  hits[j]=x;
 }
 int events=0;
 for(uint32_t k=0;k<n;k++){
  if(hits[k].dir!=race->track->gates[hits[k].index].direction)continue;
  /* Position within the accepted physics movement determines the
   * checkpoint timestamp. Multiple gates in a 60 Hz tick no longer all
   * share the end-of-frame time; observed order is preserved. */
  double f=fmax(0.0,fmin(1.0,(double)hits[k].along));
  uint64_t duration=now_ms-previous_ms;
  uint64_t offset=duration;
  if(f<1.0){
   /* The simulation's normal increment is 16-17 ms; cap the arithmetic
    * before conversion for artificially huge timestamps. */
   double part=(double)duration*f;
   if(part<(double)duration)
    offset=part<0.0?0u:(uint64_t)(part+0.5);
  }
  if(offset>duration)offset=duration;
  uint64_t crossing_ms=previous_ms+offset;
  if(hits[k].index!=race->next_gate){
   /* Out-of-order valid checkpoint => no shortcut credited. */
   race->next_gate=1;
   race->sector_valid=0; /* gap can no longer be assigned to a known sector */
   continue;
  }
  events|=CNK_RACE_GATE;
  if(race->sector_valid){
   uint64_t split=crossing_ms-race->sector_start_ms;
   uint32_t gate=hits[k].index;
   race->last_sector_ms=split;
   race->last_sector_gate=gate;
   if(split && (!race->best_sector_ms[gate]||
                split<race->best_sector_ms[gate]))
    race->best_sector_ms[gate]=split;
  }
  race->sector_start_ms=crossing_ms;
  race->sector_valid=1;
  race->last_accepted_gate=hits[k].index;
  race->next_gate=(race->next_gate+1)%race->track->gate_count;
  if(hits[k].index==0){
   race->completed_laps++;
   race->last_lap_ms=crossing_ms-race->lap_start_ms;
   if(!race->best_lap_ms||race->last_lap_ms<race->best_lap_ms)
    race->best_lap_ms=race->last_lap_ms;
   race->lap_start_ms=crossing_ms;events|=CNK_RACE_LAP;
   race->next_gate=1;
   if(race->completed_laps==race->total_laps){
    race->finished=1;race->running=0;race->finish_ms=crossing_ms;
    events|=CNK_RACE_FINISH;break;
   }
  }
 }
 return events;
}
