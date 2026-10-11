#include "cnk_projectile_threat.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
int main(void){
 cnk_item_world w={0};cnk_kart kart={0};
 assert(!cnk_kart_spawn(&kart,(cnk_vec3){0,0,0},0));
 kart.speed=8;
 cnk_projectile_threat info={0};
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&!info.incoming);
 w.missiles[0]=(cnk_item_missile){{0.25f,0,6},3.14159265f,1,1,0,1};
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info));
 assert(info.incoming&&info.kind==CNK_WEAPON_MISSILE && info.eta_seconds>0.1f && info.eta_seconds<0.5f);
 assert(info.lateral_side>0);
 /* Own projectiles, objects leaving, distant & expired threats do not warn. */
 w.missiles[0].owner=0;
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&!info.incoming);
 w.missiles[0].owner=1;w.missiles[0].heading=0;
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&!info.incoming);
 w.missiles[0].active=0;
 w.bombs[0]=(cnk_item_bomb){{-0.4f,0,4},3.14159265f,0.7f,1,1};
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&info.incoming);
 assert(info.kind==CNK_WEAPON_BOMB && info.lateral_side<0);
 w.bombs[0].position.y=6;
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&!info.incoming);
 w.bombs[0].position.y=0;w.bombs[0].lifetime=0.05f;
 assert(!cnk_projectile_threat_read(&w,0,&kart,&info)&&!info.incoming);
 /* Error is atomic. */
 w.bombs[0].lifetime=1;w.bombs[0].heading=NAN;
 info.incoming=17;
 assert(cnk_projectile_threat_read(&w,0,&kart,&info)<0 && info.incoming==17);
 puts("PASS moving-projectile predictions, own/expired/away filtering and atomic failure");
 return 0;
}
