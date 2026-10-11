#include "cnk_app_runtime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&vfs,3,3));
 assert(!cnk_app_runtime_tick(&app,1000000,CNK_UI_CONFIRM,
                             (cnk_driver_input){0},&snap));
 assert(snap.state==CNK_FRONTEND_RACE&&!snap.incoming_projectile);
 cnk_session *s=&app.frontend.game.session;
 cnk_kart *k=&s->racers[0].kart;
 k->speed=0;
 cnk_vec3 pos=k->position;
 pos.x+=sinf(k->heading)*4.5f;
 pos.z+=cosf(k->heading)*4.5f;
 s->items.missiles[0]=(cnk_item_missile){pos,k->heading+3.14159265f,1.0f,1,0,1};
 assert(!cnk_app_runtime_tick(&app,1000100,0,(cnk_driver_input){0},&snap));
 assert(snap.incoming_projectile &&
       (snap.app_events&CNK_APP_EVENT_INCOMING_PROJECTILE));
 assert(!cnk_app_runtime_tick(&app,1000200,0,(cnk_driver_input){0},&snap));
 assert(snap.incoming_projectile &&
       !(snap.app_events&CNK_APP_EVENT_INCOMING_PROJECTILE));
 s->items.missiles[0].active=0;
 assert(!cnk_app_runtime_tick(&app,1000300,0,(cnk_driver_input){0},&snap));
 assert(!snap.incoming_projectile &&
       !(snap.app_events&CNK_APP_EVENT_INCOMING_PROJECTILE));
 s->items.missiles[0].active=1;
 assert(!cnk_app_runtime_tick(&app,1000400,0,(cnk_driver_input){0},&snap));
 assert(snap.incoming_projectile &&
       (snap.app_events&CNK_APP_EVENT_INCOMING_PROJECTILE));
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 puts("PASS projectile proximity one-shot alert, no frame spam, clear/rearm");
 return 0;
}
