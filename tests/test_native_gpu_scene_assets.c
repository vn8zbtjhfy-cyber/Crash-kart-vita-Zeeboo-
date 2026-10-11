/* Test the PRODUCTION per-scene texture uploader and draw code against the
 * original user-supplied VFS and a host-side GPU call recorder.
 * Synthetic image/slot associations below test API mechanics only; they are
 * NEVER used as canonical mappings in the final game or exported files. */
#include "fufs.h"
#include "cnk_scene.h"
#include "cnk_png.h"
#include "cnk_geometry.h"
#include "cnk_render.h"
#include "cnk_camera.h"
#include "cnk_vitagl_textures.h"
#include "cnk_vitagl_scene_assets.h"
#include "cnk_vitagl_render.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int source_scene(fufs_file *vfs,uint32_t index,cnk_scene *s){
 unsigned char *data=NULL;size_t len=0;
 assert(fufs_extract(vfs,index,&data,&len)==0);
 int is_scene=len>=12&&!memcmp(data,"PIFF",4)&&!memcmp(data+8,"SCEN",4);
 if(is_scene)assert(cnk_parse_scene(data,len,s)==0);
 free(data);return is_scene;
}
static void check_scene(fufs_file *vfs,cnk_png_catalog *png,uint32_t scene_index,
                        cnk_scene *scene,unsigned *drawn){
 cnk_vitagl_textures gpu={0};cnk_camera cam={0};
 assert(scene->texture_count && scene->texture_count<=1024u);
 assert(cnk_vitagl_textures_init(&gpu,vfs,png,scene->texture_count)==0);
 cnk_test_gl_reset(0);
 assert(cnk_vitagl_scene_upload_verified(&gpu,scene,scene_index,NULL,0)==0);
 assert(!cnk_test_gl.handles_created);
 /* Find two distinct TEXTURE SLOTS mentioned by live scene materials.
  * The chosen PNGs do NOT represent actual original material assignments. */
 uint16_t slots[2]={0,0};size_t n=0;
 for(uint32_t k=0;k<scene->material_count && n<2;k++){
  uint16_t slot=scene->materials[k].texture;
  if(slot>=scene->texture_count || !scene->texture_names[slot][0])continue;
  if(n && slot==slots[0])continue;
  slots[n++]=slot;
 }
 if(!n){ cnk_vitagl_textures_free(&gpu);return; }
 cnk_texture_reference refs[2]={{0}};
 for(size_t i=0;i<n;i++){
  refs[i].scene_vfs_index=scene_index;
  refs[i].scene_resource_id=vfs->entries[scene_index].id;
  refs[i].texture_slot=slots[i];
  refs[i].original_texture_name=scene->texture_names[slots[i]];
  refs[i].png_vfs_index=png->items[i].vfs_index;
  refs[i].png_resource_id=png->items[i].vfs_id;
 }
 assert(cnk_vitagl_scene_upload_verified(&gpu,scene,scene_index,refs,n)==0);
 assert(cnk_test_gl.handles_created==n);
 for(size_t j=0;j<n;j++)assert(cnk_vitagl_textures_resolve(&gpu,slots[j])!=0);
 /* Occupied slots and stale source IDs must be rejected without new GL work. */
 uint32_t prior=cnk_test_gl.handles_created;
 assert(cnk_vitagl_scene_upload_verified(&gpu,scene,scene_index,refs,n)==-5);
 assert(cnk_test_gl.handles_created==prior);
 refs[0].scene_resource_id^=1;
 assert(cnk_vitagl_scene_upload_verified(&gpu,scene,scene_index,refs,n)==-4);
 assert(cnk_test_gl.handles_created==prior);
 refs[0].scene_resource_id^=1;
 /* Real mesh + REAL production bridge; only the texture labels are synthetic. */
 cnk_geometry geo={0};cnk_render_mesh mesh={0};
 assert(cnk_geometry_build(scene,1.f/64.f,&geo)==0);
 assert(cnk_render_mesh_build(&geo,&mesh)==0);
 assert(cnk_camera_build(&cam,(cnk_vec3){0,40,100},(cnk_vec3){0,0,0},
                         1.1f,960.f/544.f,.1f,5000.f)==0);
 cnk_test_gl.vertex_limit=mesh.vertex_count;
 unsigned calls=0;
 assert(cnk_vitagl_draw_scene(&mesh,&cam,cnk_vitagl_textures_resolve,&gpu,&calls)==0);
 assert(calls==mesh.batch_count && !cnk_test_gl.invalid);
 *drawn+=calls;
 cnk_render_mesh_free(&mesh);cnk_geometry_free(&geo);
 cnk_vitagl_textures_free(&gpu);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
}
static void check_rollback(fufs_file *vfs,const cnk_png_catalog *png,
                           const cnk_scene *s,uint32_t scene_index){
 uint16_t slots[2];size_t n=0;
 for(uint32_t i=0;i<s->texture_count && n<2;i++)if(s->texture_names[i][0])slots[n++]=i;
 assert(n==2);
 cnk_texture_reference refs[2];
 for(int i=0;i<2;i++){
  refs[i]=(cnk_texture_reference){scene_index,vfs->entries[scene_index].id,
    slots[i],s->texture_names[slots[i]],png->items[i].vfs_index,png->items[i].vfs_id};
 }
 cnk_vitagl_textures gpu={0};
 assert(cnk_vitagl_textures_init(&gpu,vfs,png,s->texture_count)==0);
 cnk_test_gl_reset(0);
 cnk_test_gl.fail_upload_on_number=2;
 assert(cnk_vitagl_scene_upload_verified(&gpu,s,scene_index,refs,2)==-6);
 assert(!cnk_vitagl_textures_resolve(&gpu,slots[0]));
 assert(!cnk_vitagl_textures_resolve(&gpu,slots[1]));
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* Retry with the same source entries after rollback; now both succeed. */
 assert(cnk_vitagl_scene_upload_verified(&gpu,s,scene_index,refs,2)==0);
 assert(cnk_vitagl_textures_resolve(&gpu,slots[0]));
 assert(cnk_vitagl_textures_resolve(&gpu,slots[1]));
 cnk_vitagl_textures_free(&gpu);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(!cnk_test_gl.invalid);
}
int main(int argc,char **argv){
 assert(argc==2);
 fufs_file vfs={0};cnk_png_catalog png={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&png)==0);
 unsigned scenes=0,draw_calls=0;
 for(uint32_t i=0;i<vfs.count;i++){
  cnk_scene s={0};
  if(source_scene(&vfs,i,&s)){
   if(!scenes)check_rollback(&vfs,&png,&s,i);
   check_scene(&vfs,&png,i,&s,&draw_calls);
   cnk_scene_free(&s);scenes++;
  }
 }
 assert(scenes==14 && draw_calls>500);
 printf("PASS GPU scene resources: %u original scenes, %u batches drawn, atomic rollback after second texture failure; mappings are SYNTHETIC TEST INPUTS only\n",scenes,draw_calls);
 cnk_png_catalog_free(&png);fufs_close(&vfs);
 return 0;
}
