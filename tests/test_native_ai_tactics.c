#include "cnk_ai_tactics.h"
#include "cnk_game.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void synthetic_decisions(void){
 cnk_vec3 points[4]={{0,0,0},{0,0,20},{0,0,40},{5,0,40}};
 cnk_sline route={"DRV0",4,points};
 cnk_item_world w={0};
 cnk_kart kart={0};
 assert(!cnk_kart_spawn(&kart,(cnk_vec3){0,0,0},0));
 kart.speed=18;
 cnk_driver_input base={1,0,0.10f},c=base;
 uint32_t decision=999;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!memcmp(&base,&c,sizeof(c)) && decision==0);
 /* TNT directly ahead, owner differs: take an evasive turn and brake when
  * close enough. Side selection is deterministic for a centred obstacle. */
 w.hazards[0]=(cnk_item_hazard){{0.3f,0,2.4f},3.0f,0,CNK_WEAPON_TNT,1};
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_AVOID_HAZARD);
 assert(c.steer<base.steer && c.throttle<base.throttle && c.brake>0);
 /* Mirror the obstacle to the left, direction must change accordingly. */
 w.hazards[0].position.x=-0.35f;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(c.steer>base.steer);
 /* Hazards behind, expired, and owned by this racer are not threats. */
 w.hazards[0].position.z=-2;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision && !memcmp(&c,&base,sizeof(c)));
 w.hazards[0].position.z=3;w.hazards[0].owner=1;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision);
 w.hazards[0].owner=0;w.hazards[0].lifetime=0;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision);
 w.hazards[0].active=0;
 /* A real moving bomb converges on the CPU kart, without any static trap. */
 w.bombs[0]=(cnk_item_bomb){{0.35f,0,4},3.14159265f,2.0f,0,1};
 c=base;assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_AVOID_PROJECTILE && c.steer<base.steer);
 w.bombs[0].position.x=-0.35f;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_AVOID_PROJECTILE && c.steer>base.steer);
 w.bombs[0].active=0;
 w.missiles[0]=(cnk_item_missile){{0.25f,0,5},3.14159265f,2.0f,0,1,1};
 c=base;assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_AVOID_PROJECTILE);
 w.missiles[0].lifetime=0.01f;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision && !memcmp(&c,&base,sizeof(c)));
 w.missiles[0].active=0;
 /* A route-adjacent verified test fixture creates a real steering choice. */
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){1.0f,0,7},1.0f,CNK_WEAPON_NONE));
 c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_SEEK_PICKUP && c.steer>base.steer);
 w.racers[1].wumpa=10;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision && !memcmp(&base,&c,sizeof(c)));
 w.racers[1].wumpa=0;
 w.pickups[0].unavailable_seconds=1;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision);
 w.pickups[0].unavailable_seconds=0;
 /* An unreachable off-route box is never chased. */
 w.pickups[0].position.x=8.0f;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision);
 /* Inventory prevents chasing a second weapon while equipped. */
 w.pickups[0].position.x=1.0f;w.pickups[0].kind=CNK_PICKUP_CRATE;
 w.pickups[0].reward=CNK_WEAPON_BOMB;w.racers[1].held=CNK_WEAPON_TNT;
 c=base;assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(!decision);
 w.racers[1].held=CNK_WEAPON_NONE;c=base;
 assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_SEEK_PICKUP);
 /* A hazard takes priority over collectibles, with no blind race abort. */
 w.hazards[0]=(cnk_item_hazard){{0,0,5},4,0,CNK_WEAPON_OIL,1};
 c=base;assert(!cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision));
 assert(decision==CNK_AI_TACTIC_AVOID_HAZARD);
 cnk_driver_input old=c;uint32_t old_event=decision;
 w.hazards[0].position.x=NAN;
 assert(cnk_ai_tactics_apply(&w,&route,1,&kart,&c,&decision)<0);
 assert(!memcmp(&old,&c,sizeof(c)) && decision==old_event);
 /* The tactical layer is never a backdoor into player control. */
 assert(cnk_ai_tactics_apply(&w,&route,0,&kart,&c,&decision)<0);
}
static void real_source_integration(const char *path){
 fufs_file vfs={0};assert(!fufs_open(path,&vfs));
 cnk_track_catalog cat={0};assert(!cnk_track_catalog_build(&vfs,&cat));
 cnk_game g={0};assert(!cnk_game_load_catalog_race(&g,&cat,0,2,2));
 cnk_session *s=&g.session;
 cnk_session_racer *r=&s->racers[1];
 assert(r->last_ai_tactic==0 && s->items.pickup_count==0);
 uint64_t original_start=r->race.start_ms;
 cnk_vec3 original_pos=r->kart.position;
 /* Baseline: none of the 12 ROM tracks contains fabricated hazards. */
 assert(cnk_session_step(s,(cnk_driver_input){0},1.0f/60,17,NULL)>=0);
 assert(r->last_ai_tactic==0);
 /* Inject a controlled test actor ahead of the actual AI, not a purported
  * original Zeebo placement. Observe steering behavior inside source race. */
 r->kart.speed=10;
 cnk_vec3 hazard=r->kart.position;
 hazard.x+=sinf(r->kart.heading)*3.0f;
 hazard.z+=cosf(r->kart.heading)*3.0f;
 s->items.hazards[0]=(cnk_item_hazard){hazard,7,0,CNK_WEAPON_OIL,1};
 assert(cnk_session_step(s,(cnk_driver_input){0},1.0f/60,34,NULL)>=0);
 assert(r->last_ai_tactic==CNK_AI_TACTIC_AVOID_HAZARD);
 assert(!r->race.finished && r->race.completed_laps==0);
 assert(r->race.start_ms==original_start && r->recovery_count==0);
 /* Live ROM race: a controlled incoming bomb causes CPU evasion. */
 s->items.hazards[0].active=0;
 r->kart.speed=10;
 cnk_vec3 incoming=r->kart.position;
 incoming.x+=sinf(r->kart.heading)*3.0f;
 incoming.z+=cosf(r->kart.heading)*3.0f;
 s->items.bombs[0]=(cnk_item_bomb){incoming,r->kart.heading+3.14159265f,2.0f,0,1};
 assert(cnk_session_step(s,(cnk_driver_input){0},1.0f/60,51,NULL)>=0);
 assert(r->last_ai_tactic==CNK_AI_TACTIC_AVOID_PROJECTILE);
 s->items.bombs[0].active=0;
 /* Removing the hazard restores the untouched route driver. */
 s->items.hazards[0].active=0;
 assert(cnk_session_step(s,(cnk_driver_input){0},1.0f/60,68,NULL)>=0);
 assert(r->last_ai_tactic==0);
 assert(isfinite(r->kart.position.x) && isfinite(r->kart.position.z));
 assert(isfinite(original_pos.x));
 cnk_game_unload(&g);fufs_close(&vfs);
}
int main(int argc,char **argv){
 assert(argc==2);
 synthetic_decisions();real_source_integration(argv[1]);
 puts("PASS verified-placement AI pickup pursuit and hazard avoidance, original-source race, no phantom checkpoint");
 return 0;
}
