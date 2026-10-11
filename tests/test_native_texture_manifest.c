#define _POSIX_C_SOURCE 200809L
#include "cnk_texture_manifest.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void save(const char *path,const char *content){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fwrite(content,1,strlen(content),f)==strlen(content));
 assert(fclose(f)==0);
}
static cnk_scene open_scene(const fufs_file *vfs,uint32_t idx){
 unsigned char *bytes=NULL;size_t size=0;cnk_scene scene={0};
 assert(fufs_extract(vfs,idx,&bytes,&size)==0);
 assert(cnk_parse_scene(bytes,size,&scene)==0);
 free(bytes);return scene;
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog cat={0};cnk_track_catalog tracks={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&cat)==0&&cat.count==317);
 assert(cnk_track_catalog_build(&vfs,&tracks)==0&&tracks.race_count==12);
 /* Data sample is SYNTHETIC and exists only in test process /tmp. */
 uint32_t scene_index=cnk_track_race_at(&tracks,0)->vfs_index;
 cnk_scene scene=open_scene(&vfs,scene_index);
 uint32_t slot=UINT32_MAX;
 for(uint32_t i=0;i<scene.texture_count;i++)if(scene.texture_names[i][0]){slot=i;break;}
 assert(slot!=UINT32_MAX);
 char path[]="/tmp/cnk_verified_mapping_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 char row[512],invalid[512];
 snprintf(row,sizeof(row),"# SYNTHETIC UNIT TEST, NOT ORIGINAL MATCHES\n%u\t%08x\t%u\t%s\t%u\t%08x\n",
  scene_index,vfs.entries[scene_index].id,slot,scene.texture_names[slot],
  cat.items[0].vfs_index,cat.items[0].vfs_id);
 save(path,row);
 cnk_texture_manifest manifest={0};
 assert(cnk_texture_manifest_read(path,&manifest)==0&&manifest.count==1);
 assert(cnk_texture_manifest_verify_all(&manifest,&vfs,&cat)==0);
 cnk_texture_reference refs[1024];size_t count=999;
 assert(cnk_texture_manifest_scene(&manifest,&scene,scene_index,&vfs,&cat,refs,1024,&count)==0);
 assert(count==1&&refs[0].png_vfs_index==cat.items[0].vfs_index);
 cnk_texture_manifest_free(&manifest);
 /* Bad original resource ID is rejected, and no outputs change. */
 snprintf(invalid,sizeof(invalid),"%u\t%08x\t%u\t%s\t%u\t%08x\n",
  scene_index,vfs.entries[scene_index].id^1u,slot,scene.texture_names[slot],
  cat.items[0].vfs_index,cat.items[0].vfs_id);
 save(path,invalid);
 assert(cnk_texture_manifest_read(path,&manifest)==0);
 assert(cnk_texture_manifest_verify_all(&manifest,&vfs,&cat)!=0);
 count=123;refs[0].texture_slot=UINT16_MAX;
 assert(cnk_texture_manifest_scene(&manifest,&scene,scene_index,&vfs,&cat,refs,1024,&count)!=0);
 assert(count==123&&refs[0].texture_slot==UINT16_MAX);
 cnk_texture_manifest_free(&manifest);
 /* Duplicate assignments are rejected at file load, atomically. */
 snprintf(invalid,sizeof(invalid),"%s%s",row,row);
 save(path,invalid);
 assert(cnk_texture_manifest_read(path,&manifest)!=0&&manifest.count==0);
 save(path,"4294967296\tffffffff\t0\tfoo.png\t1\t00000001\n");
 assert(cnk_texture_manifest_read(path,&manifest)!=0);
 save(path,"1\tffffffff\t0\tfoo.png\t1\t00000001\tEXTRA\n");
 assert(cnk_texture_manifest_read(path,&manifest)!=0);
 remove(path);
 assert(cnk_texture_manifest_read(path,&manifest)==-2);
 cnk_scene_free(&scene);
 cnk_png_catalog_free(&cat);fufs_close(&vfs);
 puts("PASS: optional source-constrained texture manifest, strict numeric/name parsing, scene+PNG validation, duplicate rejection and atomic failure (synthetic mappings never shipped)");
 return 0;
}
