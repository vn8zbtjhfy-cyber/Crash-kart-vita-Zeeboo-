#include "cnk_ai_traffic.h"
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void unit(void){
 cnk_vec3 pts[4]={{0,0,0},{0,0,20},{0,0,40},{0,0,0}};
 cnk_sline route={"DRV0",4,pts};
 cnk_kart k[3]={0};uint8_t live[3]={1,1,1};
 for(int i=0;i<3;i++)assert(!cnk_kart_spawn(&k[i],(cnk_vec3){0,0,-20},0));
 k[1].position=(cnk_vec3){0,0,0};k[1].speed=14;
 k[0].position=(cnk_vec3){0,0,3};k[0].speed=5;
 k[2].position=(cnk_vec3){12,0,4};k[2].speed=0;
 cnk_driver_input base={1,0,0.25f},next=base;uint32_t d=99;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));
 assert(d==CNK_AI_TRAFFIC_FOLLOW && next.brake>0 && next.throttle==0.30f);
 assert(next.steer==base.steer);
 /* Farther ahead: a gentle, route-bound overtaking suggestion. */
 k[0].position=(cnk_vec3){0.35f,0,6};k[0].speed=4;
 next=base;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));
 assert(d==CNK_AI_TRAFFIC_PASS && next.steer<base.steer);
 assert(next.throttle==base.throttle && next.brake==base.brake);
 /* Both overtaking corridors occupied: no unsafe pass. */
 k[2].position=(cnk_vec3){-1.65f,0,2.0f};
 next=base;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));
 assert(d==CNK_AI_TRAFFIC_PASS && next.steer>base.steer);
 k[2].position=(cnk_vec3){12,0,4};
 k[0].position.z=-10;next=base;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));
 assert(d==0 && !memcmp(&next,&base,sizeof(next)));
 k[0].position.z=3;k[0].speed=16;next=base;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));
 assert(d==0 && !memcmp(&next,&base,sizeof(next)));
 k[0].speed=0;live[0]=0;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));assert(d==0);
 live[0]=1;k[0].position.z=3;k[0].position.y=9;
 assert(!cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d));assert(d==0);
 k[0].position.y=NAN;cnk_driver_input safe=next;d=77;
 assert(cnk_ai_traffic_apply(&route,k,live,3,1,&next,&d)<0);
 assert(d==77 && !memcmp(&next,&safe,sizeof(next)));
 assert(cnk_ai_traffic_apply(&route,k,live,3,0,&next,&d)<0);
}
static void source_race(const char *path){
 fufs_file vfs={0};assert(!fufs_open(path,&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&cat,0,2,3));
 cnk_session *s=&g.session;cnk_session_racer *r=&s->racers[1];
 assert(!cnk_session_step(s,(cnk_driver_input){0},1.f/60,17,NULL));
 cnk_vec3 front=r->kart.position;
 front.x+=sinf(r->kart.heading)*2.0f;
 front.z+=cosf(r->kart.heading)*2.0f;
 s->racers[0].kart.position=front;
 s->racers[0].kart.speed=2;
 r->kart.speed=12;
 uint64_t start=r->race.start_ms;
 assert(cnk_session_step(s,(cnk_driver_input){0},1.f/60,34,NULL)>=0);
 assert(r->last_ai_tactic==CNK_AI_TRAFFIC_FOLLOW);
 assert(r->race.start_ms==start && !r->race.finished);
 cnk_game_unload(&g);fufs_close(&vfs);
}
int main(int argc,char **argv){assert(argc==2);unit();source_race(argv[1]);
 puts("PASS CPU trailing brake & defensive traffic decisions in real source race");return 0;}
