#define _POSIX_C_SOURCE 200809L
#include "cnk_item_visual.h"
#include "cnk_item_sprite_map.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
static void write_contents(const char *path,const char *contents){
 FILE *f=fopen(path,"wb");assert(f);
 assert(fputs(contents,f)>=0);assert(!fclose(f));
}
int main(int argc,char **argv){
 assert(argc==2);
 cnk_item_world w={0};cnk_item_visual_frame actors={0};
 assert(!cnk_item_visual_collect(&w,&actors)&&actors.count==0);
 assert(!cnk_items_place(&w,CNK_PICKUP_WUMPA,(cnk_vec3){1,2,3},1.0f,CNK_WEAPON_NONE));
 assert(!cnk_items_place(&w,CNK_PICKUP_CRATE,(cnk_vec3){4,5,6},1.0f,CNK_WEAPON_BOMB));
 w.bombs[1]=(cnk_item_bomb){{7,8,9},0,2,0,1};
 w.missiles[2]=(cnk_item_missile){{10,11,12},0,2,0,UINT32_MAX,1};
 w.hazards[0]=(cnk_item_hazard){{13,14,15},2,0,CNK_WEAPON_TNT,1};
 w.hazards[1]=(cnk_item_hazard){{16,17,18},2,0,CNK_WEAPON_OIL,1};
 assert(!cnk_item_visual_collect(&w,&actors)&&actors.count==6);
 for(size_t i=0;i<actors.count;i++)assert((int)actors.actors[i].sprite==(int)i);
 assert(actors.actors[3].position.z==12 && actors.actors[5].position.x==16);
 w.pickups[1].unavailable_seconds=2.0f;
 assert(!cnk_item_visual_collect(&w,&actors)&&actors.count==5);
 assert(actors.actors[1].sprite==CNK_ITEM_SPRITE_BOMB);
 cnk_item_visual_frame previous=actors;
 w.missiles[2].position.z=NAN;
 assert(cnk_item_visual_collect(&w,&actors)<0);
 assert(!memcmp(&actors,&previous,sizeof(actors))); /* atomic validation */
 w.missiles[2].position.z=12;
 w.hazards[1].kind=CNK_WEAPON_MISSILE;
 assert(cnk_item_visual_collect(&w,&actors)<0);
 assert(!memcmp(&actors,&previous,sizeof(actors)));
 w.hazards[1].kind=CNK_WEAPON_OIL;
 w.bombs[1].active=0;w.missiles[2].active=0;
 w.hazards[0].active=0;w.hazards[1].active=0;
 assert(!cnk_item_visual_collect(&w,&actors)&&actors.count==1);
 assert(actors.actors[0].sprite==CNK_ITEM_SPRITE_WUMPA);
 /* A real VFS-original PNG can only be chosen by reviewed index + FUFS ID. */
 fufs_file vfs={0};cnk_png_catalog png={0};
 assert(!fufs_open(argv[1],&vfs));
 assert(!cnk_png_catalog_build(&vfs,&png));
 assert(png.count>0);
 char path[]="/tmp/cnk49_sprite_map_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 char row[256];cnk_item_sprite_map map={0};
 snprintf(row,sizeof(row),"WUMPA\t%u\t%08x\n",
          png.items[0].vfs_index,png.items[0].vfs_id);
 write_contents(path,row);
 assert(!cnk_item_sprite_map_read(path,&vfs,&png,&map));
 assert(map.count==1&&map.present[CNK_ITEM_SPRITE_WUMPA]);
 cnk_item_sprite_map old=map;
 snprintf(row,sizeof(row),"WUMPA\t%u\t%08x\nWUMPA\t%u\t%08x\n",
          png.items[0].vfs_index,png.items[0].vfs_id,
          png.items[0].vfs_index,png.items[0].vfs_id);
 write_contents(path,row);
 assert(cnk_item_sprite_map_read(path,&vfs,&png,&map)<0);
 assert(!memcmp(&map,&old,sizeof(map)));
 snprintf(row,sizeof(row),"WUMPA\t%u\tffffffff\n",png.items[0].vfs_index);
 write_contents(path,row);
 assert(cnk_item_sprite_map_read(path,&vfs,&png,&map)<0);
 assert(!memcmp(&map,&old,sizeof(map)));
 write_contents(path,"MISSILE\t3\t00000111\tBAD\n");
 assert(cnk_item_sprite_map_read(path,&vfs,&png,&map)<0);
 assert(!memcmp(&map,&old,sizeof(map)));
 write_contents(path,"# Verified entries only\n");
 assert(!cnk_item_sprite_map_read(path,&vfs,&png,&map)&&map.count==0);
 assert(unlink(path)==0);
 cnk_png_catalog_free(&png);fufs_close(&vfs);
 puts("PASS: authoritative visible actors, atomic invalid rejection, strict original PNG binding");
 return 0;
}
