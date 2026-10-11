#include "cnk_drive.h"
#include <math.h>
#include <string.h>

#define CNK_PI 3.14159265358979323846f
static float clampf(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float norm_angle(float a){
 while(a>CNK_PI)a-=2.0f*CNK_PI;
 while(a< -CNK_PI)a+=2.0f*CNK_PI;
 return a;
}
static int finite_vec(cnk_vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
void cnk_drive_default_tuning(cnk_drive_tuning *t){
 if(!t)return;
 /* Temporary test coefficients, deliberately not disguised as Zeebo values. */
 *t=(cnk_drive_tuning){32.0f,60.0f,4.0f,30.0f,7.0f,4.0f,12.0f,24.0f,12.0f};
}
int cnk_drive_validate(const cnk_drive_tuning *t){
 if(!t)return -1;
 const float values[9]={t->acceleration,t->braking,t->drag,t->max_speed,
   t->reverse_speed,t->turn_rate,t->wheel_response,t->lookahead,t->min_ai_speed};
 for(int i=0;i<9;i++)if(!isfinite(values[i])||values[i]<0||values[i]>100000.0f)return -2;
 if(t->max_speed<=0||t->braking<=0||t->acceleration<=0||t->turn_rate<=0||
    t->wheel_response<=0||t->lookahead<=0||t->reverse_speed<=0||
    t->min_ai_speed<=0||t->min_ai_speed>t->max_speed)return -3;
 return 0;
}
int cnk_kart_spawn(cnk_kart *kart,cnk_vec3 point,float heading){
 if(!kart||!finite_vec(point)||!isfinite(heading))return -1;
 memset(kart,0,sizeof(*kart));kart->position=point;
 kart->heading=norm_angle(heading);return 0;
}
int cnk_drive_step(cnk_kart *kart,const cnk_drive_tuning *t,
                   cnk_driver_input input,float dt,const cnk_collision_grid *ground){
 if(!kart||cnk_drive_validate(t)||!finite_vec(kart->position)||
    !isfinite(kart->heading)||!isfinite(kart->speed)||!isfinite(kart->steering)||
    !isfinite(input.throttle)||!isfinite(input.brake)||!isfinite(input.steer)||
    !isfinite(dt)||dt<=0||dt>0.10f)return -1;
 input.throttle=clampf(input.throttle,0,1);
 input.brake=clampf(input.brake,0,1);
 input.steer=clampf(input.steer,-1,1);
 /* Stable integration at fixed maximum substep; small dt used identically on
  * the ARM device and in the host tests. Caller retains real elapsed time. */
 unsigned steps=(unsigned)ceilf(dt*120.0f);
 if(!steps || steps>12)return -1;
 float h=dt/(float)steps;
 for(unsigned i=0;i<steps;i++){
  float response=clampf(h*t->wheel_response,0.0f,1.0f);
  kart->steering+=(input.steer-kart->steering)*response;
  float old_speed=kart->speed;
  float drag=clampf(t->drag*h,0.0f,1000.0f);
  if(kart->speed>0)kart->speed=fmaxf(0,kart->speed-drag);
  if(kart->speed<0)kart->speed=fminf(0,kart->speed+drag);
  /* BRAKE from rest engages a controlled reverse. Gas accelerates forward
   * even while reversing. If both pedals are held, brake only slows forward
   * motion; never generate a reverse impulse from opposing inputs. The
   * coefficients are provisional, not recovered Zeebo handling values. */
  if(kart->speed>0.0f){
   kart->speed=fmaxf(0.0f,kart->speed-t->braking*input.brake*h);
  }else if(input.brake>0.0f && input.throttle<0.01f){
   kart->speed=fmaxf(-t->reverse_speed,
                     kart->speed-t->braking*0.35f*input.brake*h);
  }
  kart->speed+=t->acceleration*input.throttle*h;
  kart->speed=clampf(kart->speed,-t->reverse_speed,t->max_speed);
  float normal_speed=fminf(1.0f,fabsf((old_speed+kart->speed)*0.5f)/(t->max_speed*0.3f));
  float turning=kart->steering*t->turn_rate*normal_speed*h;
  if(kart->speed<0)turning=-turning;
  kart->heading=norm_angle(kart->heading+turning);
  kart->position.x+=sinf(kart->heading)*kart->speed*h;
  kart->position.z+=cosf(kart->heading)*kart->speed*h;
  if(ground){
   float y;
   /* Source mesh data as ground-height hint, not kart suspension dynamics. */
   if(!cnk_collision_surface(ground,kart->position.x,kart->position.z,
                              kart->position.y,&y,NULL))kart->position.y=y;
  }
 }
 kart->ticks++;
 if(!finite_vec(kart->position)||!isfinite(kart->speed))return -2;
 return 0;
}
int cnk_ai_init(cnk_ai_driver *ai,const cnk_sline *route,int closed,
                const cnk_drive_tuning *t){
 if(!ai||!route||!route->points||route->count<3||cnk_drive_validate(t))return -1;
 float distance=cnk_nav_length(route,closed);
 if(!isfinite(distance)||distance<=0)return -2;
 *ai=(cnk_ai_driver){0};ai->route=route;ai->closed=!!closed;
 ai->lookahead=t->lookahead;ai->target_speed=t->max_speed;
 ai->route_length=distance;
 return 0;
}
/* Avoid switching onto a nearby crossing segment, as in two source
 * circuits with loops placed close to themselves. Only project onto source
 * segments within a small arc-length window around previous progress.
 * This is a route-navigation policy, NOT reverse engineered opponent AI. */
static int local_progress(cnk_ai_driver *ai,cnk_vec3 pos,float *fraction,float *distance){
 const cnk_sline *route=ai->route;
 float total=ai->route_length;
 float best=1.0e30f,best_progress=0.0f;
 float travelled=0.0f;
 uint32_t n=ai->closed?route->count:route->count-1;
 if(total<=0)return -1;
 for(uint32_t i=0;i<n;i++){
  cnk_vec3 a=route->points[i],b=route->points[(i+1)%route->count];
  float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z;
  float len2=dx*dx+dy*dy+dz*dz;
  float len=sqrtf(len2);
  float u=len2>0?clampf(((pos.x-a.x)*dx+(pos.y-a.y)*dy+(pos.z-a.z)*dz)/len2,0,1):0;
  float projection=travelled+u*len;
  float unwrapped=projection;
  if(ai->progress_initialized){
   if(ai->closed)unwrapped+=total*floorf((ai->progress_units-projection)/total+0.5f);
   float delta=unwrapped-ai->progress_units;
   if(delta < -fmaxf(8.0f,ai->lookahead*0.45f) ||
      delta > fmaxf(50.0f,ai->lookahead*2.5f)){
    travelled+=len;
    continue;
   }
  }
  float ex=pos.x-(a.x+u*dx),ey=pos.y-(a.y+u*dy),ez=pos.z-(a.z+u*dz);
  float dist2=ex*ex+ey*ey+ez*ez;
  if(dist2<best){best=dist2;best_progress=unwrapped;}
  travelled+=len;
 }
 if(best==1.0e30f)return -2;
 ai->progress_units=best_progress;
 ai->progress_initialized=1;
 *fraction=best_progress/total;
 *fraction-=floorf(*fraction);
 *distance=sqrtf(best);
 return 0;
}
int cnk_ai_controls(cnk_ai_driver *ai,const cnk_kart *kart,
                    const cnk_drive_tuning *t,cnk_driver_input *out){
 if(!ai||!kart||!out||cnk_drive_validate(t)||!ai->route||
    !finite_vec(kart->position)||!isfinite(kart->heading)||!isfinite(kart->speed))return -1;
 float fraction=0.0f,deviation=0.0f;
 if(local_progress(ai,kart->position,&fraction,&deviation))return -2;
 /* If a kart was physically displaced far away from the expected corridor,
  * allow reacquisition rather than spinning forever around a stale target.
  * A guarded threshold avoids lane-switching near ordinary intersections. */
 if(deviation>fmaxf(40.0f,ai->lookahead*2.0f)){
  cnk_nav_projection global;
  if(!cnk_nav_nearest(ai->route,kart->position,ai->closed,&global)&&
     global.distance+12.0f<deviation){
   ai->progress_units=global.fraction*ai->route_length;
   fraction=global.fraction;deviation=global.distance;
  }
 }
 float total=ai->route_length;
 if(total<0.01f)return -2;
 ai->route_fraction=fraction;ai->route_distance=deviation;
 float ahead=clampf(ai->lookahead,3.0f,120.0f);
 cnk_vec3 target,far;
 if(cnk_nav_sample_with_length(ai->route,fraction+ahead/total,ai->closed,total,&target)||
    cnk_nav_sample_with_length(ai->route,fraction+ahead*2.0f/total,ai->closed,total,&far))return -3;
 float target_heading=atan2f(target.x-kart->position.x,target.z-kart->position.z);
 float relative=norm_angle(target_heading-kart->heading);
 /* Stronger response for distant offset; constrained to steering [-1,1]. */
 float steer=clampf(relative*1.3f,-1.0f,1.0f);
 float bend=norm_angle(atan2f(far.x-target.x,far.z-target.z)-
                      atan2f(target.x-kart->position.x,target.z-kart->position.z));
 float severity=fmaxf(fabsf(relative),fabsf(bend));
 float target_speed=t->max_speed*(1.0f-clampf(severity/2.2f,0.0f,0.72f));
 target_speed=fmaxf(t->min_ai_speed,target_speed);
 ai->target_speed=target_speed;
 out->steer=steer;
 out->throttle=kart->speed<target_speed?1.0f:0.0f;
 out->brake=kart->speed>target_speed+2.0f?0.6f:0.0f;
 return 0;
}
int cnk_drive_race_step(cnk_kart *kart,const cnk_drive_tuning *t,
                        cnk_driver_input input,float dt,
                        const cnk_collision_grid *surface,
                        cnk_race *race,uint64_t now_ms){
 if(!kart)return -1;
 cnk_vec3 before=kart->position;
 int rc=cnk_drive_step(kart,t,input,dt,surface);
 if(rc)return rc;
 return race?cnk_race_advance(race,before,kart->position,now_ms):0;
}

void cnk_world_collision_default(cnk_world_collision_tuning *out){
 if(out)*out=(cnk_world_collision_tuning){0.75f,0.20f,0.55f,0};
}
int cnk_world_collision_validate(const cnk_world_collision_tuning *c){
 if(!c||!isfinite(c->probe_height)||!isfinite(c->clearance)||
    !isfinite(c->max_abs_normal_y)||c->probe_height<0.02f||
    c->probe_height>20.0f||c->clearance<0.0f||c->clearance>5.0f||
    c->max_abs_normal_y<0.0f||c->max_abs_normal_y>1.0f||
    (c->slide_on_contact!=0&&c->slide_on_contact!=1))return -1;
 return 0;
}
int cnk_drive_step_world(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world){
 if(!kart||!surface||cnk_world_collision_validate(world))return -1;
 /* Compute candidate first. Malformed geometry leaves the kart unchanged. */
 cnk_kart proposed=*kart;
 int rc=cnk_drive_step(&proposed,t,input,dt,surface);
 if(rc)return rc;
 cnk_vec3 from={kart->position.x,kart->position.y+world->probe_height,kart->position.z};
 cnk_vec3 to={proposed.position.x,proposed.position.y+world->probe_height,proposed.position.z};
 float dx=to.x-from.x,dy=to.y-from.y,dz=to.z-from.z;
 float length=sqrtf(dx*dx+dy*dy+dz*dz);
 if(!isfinite(length))return -2;
 if(length>0.000001f){
  cnk_segment_hit hit={0};
  rc=cnk_collision_segment_cast_steep(surface,from,to,world->max_abs_normal_y,&hit);
  if(rc!=-2 && rc!=0)return -3;
  if(rc==0){
   float fraction=clampf(hit.fraction-world->clearance/length,0.0f,1.0f);
   proposed.position.x=kart->position.x+(proposed.position.x-kart->position.x)*fraction;
   proposed.position.y=kart->position.y+(proposed.position.y-kart->position.y)*fraction;
   proposed.position.z=kart->position.z+(proposed.position.z-kart->position.z)*fraction;
   if(world->slide_on_contact){
    /* Remove the wall-normal component from the REMAINING movement.
     * This is a deliberately provisional response, not a recovered
     * coefficient or tire model. The first sweep still guards against
     * tunneling; a second swept segment guards the projected movement. */
    float remain=1.0f-fraction;
    cnk_vec3 residual={dx*remain,dy*remain,dz*remain};
    float dot=residual.x*hit.normal.x+residual.y*hit.normal.y+
              residual.z*hit.normal.z;
    cnk_vec3 tangent={residual.x-dot*hit.normal.x,
                      residual.y-dot*hit.normal.y,
                      residual.z-dot*hit.normal.z};
    cnk_vec3 slide_from={proposed.position.x,
                         proposed.position.y+world->probe_height,
                         proposed.position.z};
    cnk_vec3 slide_to={slide_from.x+tangent.x,slide_from.y+tangent.y,
                       slide_from.z+tangent.z};
    float slide_len=sqrtf(tangent.x*tangent.x+tangent.y*tangent.y+
                           tangent.z*tangent.z);
    if(!isfinite(slide_len))return -2;
    if(slide_len>0.000001f){
     cnk_segment_hit next={0};
     int second=cnk_collision_segment_cast_steep(surface,slide_from,slide_to,
                                    world->max_abs_normal_y,&next);
     if(second!=-2&&second!=0)return -3;
     float secondary_fraction=second==0 ?
                 clampf(next.fraction-world->clearance/slide_len,0.0f,1.0f) : 1.0f;
     proposed.position.x+=tangent.x*secondary_fraction;
     proposed.position.y+=tangent.y*secondary_fraction;
     proposed.position.z+=tangent.z*secondary_fraction;
    }
    /* Kart faces the original steering direction. Only the movement along
     * the contact plane is retained, without an artificial bounce. */
    float planar_len=sqrtf(dx*dx+dz*dz);
    float tangent_planar=sqrtf(tangent.x*tangent.x+tangent.z*tangent.z);
    proposed.speed=planar_len>0.000001f ?
       proposed.speed*clampf(tangent_planar/(planar_len*remain+1e-8f),0.0f,1.0f):0.0f;
   }else{
    proposed.speed=0.0f; /* v20's existing stop response, still default */
   }
   *kart=proposed;
   return 1;
  }
 }
 *kart=proposed;
 return 0;
}
int cnk_drive_race_step_world_report(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world,cnk_race *race,uint64_t now_ms,
 int *contact){
 if(!kart)return -1;
 cnk_vec3 before=kart->position;
 int rc=cnk_drive_step_world(kart,t,input,dt,surface,world);
 if(rc<0)return rc;
 if(contact)*contact=(rc==1);
 return race?cnk_race_advance(race,before,kart->position,now_ms):0;
}
int cnk_drive_race_step_world(cnk_kart *kart,const cnk_drive_tuning *t,
 cnk_driver_input input,float dt,const cnk_collision_grid *surface,
 const cnk_world_collision_tuning *world,cnk_race *race,uint64_t now_ms){
 return cnk_drive_race_step_world_report(kart,t,input,dt,surface,world,race,now_ms,NULL);
}
