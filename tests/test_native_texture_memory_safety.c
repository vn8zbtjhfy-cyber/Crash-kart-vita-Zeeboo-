#define _POSIX_C_SOURCE 200809L
/* Advance 100: verify source-backed texture validation using tiny heap
 * strings and bounded caller buffers. Neither test string is asserted to
 * be an authentic original material-to-PNG association. */
#include "cnk_texture_manifest.h"
#include "cnk_track_catalog.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char **argv){
 assert(argc==2);
 /* Short heap strings must not be read 256 bytes beyond their allocation. */
 char *a=malloc(2),*b=malloc(2);
 assert(a&&b);strcpy(a,"A");strcpy(b,"A");
 cnk_texture_reference r[2]={{0}};
 r[0].original_texture_name=a;r[0].png_resource_id=10;r[0].png_vfs_index=3;
 r[1].original_texture_name=b;r[1].png_resource_id=11;r[1].png_vfs_index=4;
 assert(cnk_texture_map_consistency(r,1)==0);
 assert(cnk_texture_map_consistency(r,2)==-2);
 free(a);free(b);

 fufs_file vfs={0};cnk_track_catalog catalog={0};cnk_png_catalog png={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_track_catalog_build(&vfs,&catalog)==0);
 assert(cnk_png_catalog_build(&vfs,&png)==0);
 const uint32_t idx=cnk_track_race_at(&catalog,0)->vfs_index;
 unsigned char *bytes=NULL;size_t size=0;cnk_scene scene={0};
 assert(fufs_extract(&vfs,idx,&bytes,&size)==0);
 assert(cnk_parse_scene(bytes,size,&scene)==0);free(bytes);
 uint32_t slot=UINT32_MAX;
 for(uint32_t i=0;i<scene.texture_count;i++)
  if(scene.texture_names[i][0]){slot=i;break;}
 assert(slot!=UINT32_MAX);
 char name[]="/tmp/cnk100_manifest_XXXXXX";
 int fd=mkstemp(name);assert(fd>=0);
 FILE *f=fdopen(fd,"wb");assert(f);
 assert(fprintf(f,"%u\t%08x\t%u\t%s\t%u\t%08x\n",idx,
     vfs.entries[idx].id,slot,scene.texture_names[slot],
     png.items[0].vfs_index,png.items[0].vfs_id)>0);
 assert(fclose(f)==0);
 cnk_texture_manifest manifest={0};
 assert(cnk_texture_manifest_read(name,&manifest)==0);
 cnk_texture_reference one={0};one.texture_slot=UINT16_MAX;
 size_t count=999u;
 /* Insufficient capacity must not modify caller's result or count. */
 assert(cnk_texture_manifest_scene(&manifest,&scene,idx,&vfs,&png,
                      NULL,0,&count)==-2);
 assert(count==999u);
 assert(cnk_texture_manifest_scene(&manifest,&scene,idx,&vfs,&png,
                      &one,1,&count)==0);
 assert(count==1&&one.texture_slot==slot);
 cnk_texture_reference invalid={0};invalid.texture_slot=UINT16_MAX;
 count=999u;
 /* Corrupt the resource ID and verify atomic failure. */
 manifest.rows[0].scene_resource_id^=1u;
 assert(cnk_texture_manifest_scene(&manifest,&scene,idx,&vfs,&png,
                      &invalid,1,&count)<0);
 assert(count==999u&&invalid.texture_slot==UINT16_MAX);
 cnk_texture_manifest_free(&manifest);
 /* A scene with no associations needs no scratch memory. */
 assert(cnk_texture_manifest_scene(&manifest,&scene,idx,&vfs,&png,
                      NULL,0,&count)==0&&count==0);
 remove(name);
 cnk_scene_free(&scene);cnk_png_catalog_free(&png);fufs_close(&vfs);
 puts("PASS: short heap material names, scene-size scratch, bounded capacity and atomic invalid-map rejection");
 return 0;
}
