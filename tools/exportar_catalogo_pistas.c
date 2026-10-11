#include "cnk_track_catalog.h"
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 fufs_file f={0};cnk_track_catalog c={0};
 if(fufs_open(argv[1],&f))return 3;
 int rc=cnk_track_catalog_build(&f,&c);
 if(rc){fufs_close(&f);return 4;}
 puts("scene_vfs_index,scene_resource_id,classification,route_points,checkpoints,route_length,original_start_x,original_start_y,original_start_z,original_heading_radians");
 for(uint32_t i=0;i<c.count;i++){
  const cnk_track_entry *e=&c.entries[i];
  printf("%u,%08x,%s,%u,%u,%.6f,",e->vfs_index,e->resource_id,
         e->kind==CNK_TRACK_RACE?"RACE_ROUTE_VALIDATED":"OTHER_SCENE",
         e->drive_points,e->gates,e->route_length);
  if(e->has_original_start)
   printf("%.6f,%.6f,%.6f,%.6f\n",e->original_start.x,
          e->original_start.y,e->original_start.z,e->original_heading);
  else puts(",,,");
 }
 fufs_close(&f);
 return 0;
}
