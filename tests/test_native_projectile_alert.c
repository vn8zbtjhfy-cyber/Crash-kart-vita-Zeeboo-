#include "cnk_app_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};assert(!cnk_app_runtime_init(&app,&vfs,3,3));
 cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(!snap.has_hud&&!snap.incoming_projectile);
 assert(!cnk_app_runtime_tick(&app,1000000,CNK_UI_CONFIRM,
                               (cnk_driver_input){0},&snap));
 assert(snap.has_hud&&!snap.incoming_projectile);
 cnk_session *s=&app.frontend.game.session;
 cnk_kart *player=&s->racers[0].kart;
 player->speed=0;
 cnk_vec3 pos=player->position;
 pos.x+=sinf(player->heading)*5.0f+0.4f;
 pos.z+=cosf(player->heading)*5.0f;
 s->items.missiles[0]=(cnk_item_missile){pos,player->heading+3.14159265f,
                                         1.0f,1,0,1};
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(snap.incoming_projectile&&snap.incoming_kind==CNK_WEAPON_MISSILE);
 assert(snap.incoming_eta_ms>250&&snap.incoming_eta_ms<450);
 assert(snap.active_missiles==1);
 /* Own weapons do not create an alert, even if the visual counter includes them. */
 s->items.missiles[0].owner=0;
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(!snap.incoming_projectile&&snap.active_missiles==1);
 s->items.missiles[0].owner=1;
 s->items.missiles[0].active=0;
 assert(!cnk_app_runtime_snapshot(&app,&snap));
 assert(!snap.incoming_projectile&&snap.active_missiles==0);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS source-loaded native runtime incoming projectile alert and instant clear");
 return 0;
}
