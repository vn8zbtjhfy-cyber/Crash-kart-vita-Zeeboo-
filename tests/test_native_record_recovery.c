#include "cnk_app_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void damage(const char *file){
 FILE *f=fopen(file,"r+b");assert(f);
 assert(!fseek(f,24,SEEK_SET));assert(fputc(0xef,f)!=EOF);
 assert(!fclose(f));
}
int main(int argc,char **argv){
 assert(argc==2);
 char path[200],backup[208];
 snprintf(path,sizeof(path),"/tmp/cnk75_recovery_%lu.dat",(unsigned long)getpid());
 snprintf(backup,sizeof(backup),"%s.bak",path);
 remove(path);remove(backup);
 cnk_records a={0},b={0},c={0},readback={0};
 assert(cnk_records_update(&a,777,3,CNK_RECORDS_NORMAL,50000,160000)==1);
 assert(!cnk_records_save(&a,path));
 assert(cnk_records_load_backup(&readback,path)<0); /* first write: no backup */
 b=a;
 assert(cnk_records_update(&b,777,3,CNK_RECORDS_NORMAL,40000,145000)==1);
 assert(!cnk_records_save(&b,path));
 assert(!cnk_records_load(&readback,path)&&readback.entries[0].best_lap_ms==40000);
 assert(!cnk_records_load_backup(&readback,path)&&readback.entries[0].best_lap_ms==50000);
 damage(path);
 readback=(cnk_records){0};
 assert(cnk_records_load(&readback,path)<0 && !readback.count); /* strict read */
 assert(!cnk_records_load_backup(&readback,path)&&readback.entries[0].best_lap_ms==50000);
 fufs_file vfs={0};assert(!fufs_open(argv[1],&vfs));
 cnk_app_runtime app={0};assert(!cnk_app_runtime_init(&app,&vfs,3,2));
 assert(!cnk_app_runtime_set_records_path(&app,path));
 assert(app.records.count==1 && app.records.entries[0].best_lap_ms==50000);
 cnk_app_runtime_close(&app);fufs_close(&vfs);
 /* Saving a repaired record must never replace the healthy backup with
  * the corrupted primary. */
 c=readback;
 assert(cnk_records_update(&c,777,3,CNK_RECORDS_NORMAL,38000,0)==1);
 assert(!cnk_records_save(&c,path));
 assert(!cnk_records_load(&readback,path)&&readback.entries[0].best_lap_ms==38000);
 assert(!cnk_records_load_backup(&readback,path)&&readback.entries[0].best_lap_ms==50000);
 /* Two subsequent good saves rotate the backup to the previous good state. */
 assert(!cnk_records_save(&c,path));
 assert(!cnk_records_load_backup(&readback,path)&&readback.entries[0].best_lap_ms==38000);
 damage(backup);
 readback=c;
 assert(cnk_records_load_backup(&readback,path)<0);
 assert(!memcmp(&readback,&c,sizeof(c))); /* no partial output on corruption */
 remove(path);remove(backup);
 puts("PASS: checksum-backed save rotation, corrupt primary recovery, backup preservation, portable Vita frontend fallback");
 return 0;
}
