#include "cnk_collision_probe.h"
#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void put(const char *path,const char *body){
 FILE *f=fopen(path,"wb");assert(f);assert(fwrite(body,1,strlen(body),f)==strlen(body));assert(!fclose(f));
}
int main(int argc,char **argv){
 assert(argc==2);
 const char *file="cnk_test_collision_probe.tmp";
 cnk_collision_probe v={1,1};
 remove(file);
 assert(cnk_collision_probe_read(file,&v)==-2);
 assert(v.walls==1&&v.karts==1);
 put(file,"# collision experiment\nWALLS\t1\nKARTS\t0\n");
 assert(!cnk_collision_probe_read(file,&v));
 assert(v.walls==1&&v.karts==0);
 put(file,"WALLS\t1\nKARTS\t1\nWALLS\t0\n");
 assert(cnk_collision_probe_read(file,&v)<0 && v.walls==1&&v.karts==0);
 put(file,"WALLS\t1\nKARTS\t3\n");
 assert(cnk_collision_probe_read(file,&v)<0 && v.walls==1&&v.karts==0);
 put(file,"WALLS\t0\nKARTS\t1\n");
 assert(!cnk_collision_probe_read(file,&v));
 assert(v.walls==0&&v.karts==1);
 assert(!remove(file));
 fufs_file source={0};assert(!fufs_open(argv[1],&source));
 cnk_app_runtime app={0};
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!app.collision_probe.walls&&!app.collision_probe.karts);
 assert(cnk_app_runtime_set_collision_probe(&app,(cnk_collision_probe){1,2})<0);
 assert(!cnk_app_runtime_set_collision_probe(&app,(cnk_collision_probe){1,1}));
 cnk_app_snapshot snap={0};
 /* Genuine native runtime transition, not directly modifying session flags. */
 assert(!cnk_app_runtime_tick(&app,1000000,CNK_UI_CONFIRM,(cnk_driver_input){0},&snap));
 assert(app.frontend.game.loaded);
 assert(app.frontend.game.session.provisional_walls_enabled==1);
 assert(app.frontend.game.session.provisional_kart_contacts_enabled==1);
 assert(cnk_app_runtime_set_collision_probe(&app,(cnk_collision_probe){0,0})<0);
 /* Advance countdown and verify collision options survive in a live race. */
 for(int i=0;i<190;i++){
  assert(!cnk_app_runtime_tick(&app,1000000ULL+(uint64_t)(i+1)*16667ULL,0,
    (cnk_driver_input){.throttle=1},&snap));
 }
 assert(app.frontend.game.simulation_frames>0);
 assert(app.frontend.game.session.provisional_walls_enabled==1);
 assert(app.frontend.game.session.provisional_kart_contacts_enabled==1);
 /* Restart through real UI actions; the runtime must reapply both flags. */
 assert(!cnk_app_runtime_tick(&app,1000000ULL+191ULL*16667ULL,CNK_UI_PAUSE,
    (cnk_driver_input){0},&snap));
 assert(app.frontend.state==CNK_FRONTEND_PAUSE);
 assert(!cnk_app_runtime_tick(&app,1000000ULL+192ULL*16667ULL,CNK_UI_RESTART,
    (cnk_driver_input){0},&snap));
 assert(app.frontend.state==CNK_FRONTEND_RACE);
 assert(app.frontend.game.simulation_frames==0);
 assert(app.frontend.game.session.provisional_walls_enabled==1);
 assert(app.frontend.game.session.provisional_kart_contacts_enabled==1);
 cnk_app_runtime_close(&app);
 assert(!cnk_app_runtime_init(&app,&source,3,2));
 assert(!app.collision_probe.walls&&!app.collision_probe.karts);
 cnk_app_runtime_close(&app);
 fufs_close(&source);
 puts("PASS optional collision probe: strict parser, no partial overwrite, default off, native activation");
 return 0;
}
