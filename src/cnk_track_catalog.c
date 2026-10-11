#include "cnk_track_catalog.h"
#include "cnk_race.h"
#include "cnk_nav.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

int cnk_track_catalog_build(const fufs_file *vfs,cnk_track_catalog *out){
 cnk_track_catalog work={0};
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!vfs||!vfs->data||!vfs->entries||!vfs->count)return -2;
 work.source=vfs;
 for(uint32_t i=0;i<vfs->count;i++){
  unsigned char *data=NULL;size_t length=0;
  if(fufs_extract(vfs,i,&data,&length))return -3;
  int scene_file=length>=12 && !memcmp(data,"PIFF",4) &&
                 !memcmp(data+8,"SCEN",4);
  if(!scene_file){free(data);continue;}
  if(work.count>=CNK_MAX_SOURCE_SCENES){free(data);return -4;}
  cnk_scene scene={0};
  int parsed=cnk_parse_scene(data,length,&scene);
  free(data);
  if(parsed){cnk_scene_free(&scene);return -5;}
  cnk_track_entry *entry=&work.entries[work.count];
  entry->vfs_index=i;
  entry->resource_id=vfs->entries[i].id;
  const cnk_sline *drive=cnk_scene_line(&scene,"DRV0");
  if(drive && drive->points && drive->count>=3){
   entry->drive_points=drive->count;
   entry->route_length=cnk_nav_length(drive,1);
  }
  float heading=0;
  if(cnk_scene_start_pose(&scene,&entry->original_start,&heading)==0){
   entry->original_heading=heading;
   entry->has_original_start=1;
  }
  cnk_race_track track={0};
  /* This classification is intentionally conservative. Do not label all
   * SCEN files as a usable race based on their graphics alone. */
  if(entry->has_original_start && isfinite(entry->route_length) &&
     entry->route_length>1 && !cnk_race_track_build(&scene,&track)){
   entry->kind=CNK_TRACK_RACE;
   entry->gates=track.gate_count;
   work.race_count++;
  }else entry->kind=CNK_TRACK_OTHER;
  cnk_scene_free(&scene);
  work.count++;
 }
 if(!work.count)return -6;
 *out=work;
 return 0;
}
const cnk_track_entry *cnk_track_race_at(const cnk_track_catalog *c,uint32_t idx){
 if(!c||!c->source||idx>=c->race_count)return NULL;
 for(uint32_t i=0;i<c->count;i++)if(c->entries[i].kind==CNK_TRACK_RACE){
  if(!idx)return &c->entries[i];
  idx--;
 }
 return NULL;
}
const cnk_track_entry *cnk_track_find(const cnk_track_catalog *c,uint32_t index){
 if(!c||!c->source)return NULL;
 for(uint32_t i=0;i<c->count;i++)if(c->entries[i].vfs_index==index)
  return &c->entries[i];
 return NULL;
}
