#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void frame(cnk_app_runtime *app,uint64_t *t,uint32_t button,cnk_app_snapshot *s){
 *t+=16667;
 assert(!cnk_app_runtime_tick(app,*t,button,(cnk_driver_input){0},s));
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};cnk_app_snapshot sn={0};
 assert(!cnk_app_runtime_init(&app,&vfs,1,3));
 const cnk_track_entry *race=cnk_frontend_selected(&app.frontend);
 assert(race && race->resource_id!=0);
 const uint32_t source_id=race->resource_id, source_index=race->vfs_index;
 char path[]="/tmp/cnk47_item_layout_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *file=fdopen(fd,"wb");assert(file);
 fprintf(file,"# VERIFIED TEST FIXTURE (not real locations)\n");
 fprintf(file,"%u\tWUMPA\t0.0\t0.0\t0.0\t1\tNONE\n",source_id);
 fprintf(file,"%u\tCRATE\t2.0\t0.0\t3.0\t1.2\tBOMB\n",source_id);
 fprintf(file,"%u\tCRATE\t4.0\t0.0\t5.0\t1.2\tTURBO\n",source_id+1u);
 assert(!fclose(file));
 cnk_item_layout layout={0};
 assert(!cnk_item_layout_read(path,&layout));
 assert(layout.count==3);
 assert(!cnk_app_runtime_set_item_layout(&app,&layout));
 uint64_t t=1000000;
 assert(!cnk_app_runtime_tick(&app,t,0,(cnk_driver_input){0},&sn));
 frame(&app,&t,CNK_UI_CONFIRM,&sn);
 assert(sn.state==CNK_FRONTEND_RACE && sn.placed_pickups==2);
 assert(app.frontend.game.session.items.pickups[0].kind==CNK_PICKUP_WUMPA);
 assert(app.frontend.game.session.items.pickups[1].reward==CNK_WEAPON_BOMB);
 assert(cnk_app_runtime_set_item_layout(&app,&layout)<0); /* cannot swap mid-race */
 frame(&app,&t,CNK_UI_PAUSE,&sn);frame(&app,&t,0,&sn);
 frame(&app,&t,CNK_UI_RESTART,&sn);
 assert(sn.placed_pickups==2); /* reload once, no duplicate items */
 frame(&app,&t,0,&sn);
 frame(&app,&t,CNK_UI_PAUSE,&sn);frame(&app,&t,0,&sn);
 frame(&app,&t,CNK_UI_CANCEL,&sn);
 assert(sn.state==CNK_FRONTEND_SELECT);
 cnk_app_runtime_close(&app);
 /* Reject corrupt placement documents atomically. */
 file=fopen(path,"wb");assert(file);
 fprintf(file,"99\tWUMPA\tnan\t0\t0\t1\tNONE\n");
 assert(!fclose(file));
 cnk_item_layout unchanged=layout;
 assert(cnk_item_layout_read(path,&unchanged)<0);
 assert(unchanged.count==layout.count);
 file=fopen(path,"wb");assert(file);
 fprintf(file,"99\tCRATE\t0\t0\t0\t1\tNONE\n");
 assert(!fclose(file));
 assert(cnk_item_layout_read(path,&unchanged)<0);
 cnk_game game={0};
 assert(!cnk_game_load_original_start(&game,&vfs,source_index,1,3));
 cnk_item_layout too_many={0};
 too_many.count=33;
 for(int i=0;i<33;i++){
  too_many.entries[i].scene_resource_id=source_id;
  too_many.entries[i].pickup.kind=CNK_PICKUP_WUMPA;
  too_many.entries[i].pickup.radius=1;
 }
 assert(cnk_item_layout_apply(&too_many,&game)<0);
 assert(game.session.items.pickup_count==0);
 cnk_game_unload(&game);
 assert(!unlink(path));
 fufs_close(&vfs);
 puts("PASS: verified source-ID item layouts, restart, malformed coordinates, atomic oversized rejection");
}
