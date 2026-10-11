#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static void tick(cnk_app_runtime *app,uint64_t *t,uint32_t b,cnk_app_snapshot *snap){
 *t+=16667;
 assert(cnk_app_runtime_tick(app,*t,b,(cnk_driver_input){0},snap)>=0);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file rom={0};assert(!fufs_open(argv[1],&rom));
 cnk_app_runtime app={0};cnk_app_snapshot snap={0};
 assert(!cnk_app_runtime_init(&app,&rom,2,2));
 const cnk_track_entry *entry=cnk_frontend_selected(&app.frontend);
 assert(entry&&entry->resource_id);
 const uint32_t source_id=entry->resource_id,source_index=entry->vfs_index;
 char path[]="/tmp/cnk79_pad_map_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *fp=fdopen(fd,"wb");assert(fp);
 fprintf(fp,"# TEST POINTS ONLY -- NOT VERIFIED ORIGINAL PAD LOCATIONS\n");
 fprintf(fp,"%u\t0\t0\t0\t1\n",source_id);
 fprintf(fp,"%u\t100\t0\t100\t1\n",source_id);
 fprintf(fp,"%u\t300\t0\t300\t1\n",source_id+1u);
 assert(!fclose(fp));
 cnk_pad_layout map={0};assert(!cnk_pad_layout_read(path,&map));
 assert(map.count==3);
 assert(!cnk_app_runtime_set_pad_layout(&app,&map));
 uint64_t clock=1000000;
 assert(!cnk_app_runtime_tick(&app,clock,0,(cnk_driver_input){0},&snap));
 tick(&app,&clock,CNK_UI_CONFIRM,&snap);
 assert(snap.state==CNK_FRONTEND_RACE && snap.placed_pads==2);
 assert(app.frontend.game.session.pads.count==2);
 assert(cnk_app_runtime_set_pad_layout(&app,&map)<0);
 tick(&app,&clock,CNK_UI_PAUSE,&snap);tick(&app,&clock,0,&snap);
 tick(&app,&clock,CNK_UI_RESTART,&snap);
 assert(snap.placed_pads==2); /* no duplication after restart */
 tick(&app,&clock,0,&snap);
 tick(&app,&clock,CNK_UI_PAUSE,&snap);tick(&app,&clock,0,&snap);
 tick(&app,&clock,CNK_UI_CANCEL,&snap);
 assert(snap.state==CNK_FRONTEND_SELECT);
 cnk_app_runtime_close(&app);
 /* Malformed files cannot change the previous valid layout. */
 fp=fopen(path,"wb");assert(fp);
 fputs("99\tNaN\t0\t0\t1\n",fp);assert(!fclose(fp));
 cnk_pad_layout old=map;
 assert(cnk_pad_layout_read(path,&old)<0 && old.count==map.count);
 fp=fopen(path,"wb");assert(fp);
 fputs("99\t0\t0\t0\t1\textra\n",fp);assert(!fclose(fp));
 assert(cnk_pad_layout_read(path,&old)<0);
 /* Two overlapping pads in a single source scene must be rejected,
  * not leave a partial modified session. */
 cnk_game game={0};
 assert(!cnk_game_load_original_start(&game,&rom,source_index,2,2));
 cnk_pad_layout bad={0};
 bad.count=2;
 bad.entries[0]=(cnk_pad_layout_entry){source_id,{0,0,0},1};
 bad.entries[1]=(cnk_pad_layout_entry){source_id,{0,0,0},1};
 assert(cnk_pad_layout_apply(&bad,&game)<0);
 assert(game.session.pads.count==0);
 cnk_game_unload(&game);
 assert(!unlink(path));fufs_close(&rom);
 puts("PASS: verified source-ID boost pad sidecar, restart, atomic loading, corruption rejection");
 return 0;
}
