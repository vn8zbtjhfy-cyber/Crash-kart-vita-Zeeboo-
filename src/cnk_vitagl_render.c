#include "cnk_vitagl_render.h"
#include "cnk_frustum.h"
#include <vitaGL.h>
#include <stddef.h>
#include <limits.h>
#include <math.h>
/* Native Vita GPU rendering using hardware-accelerated vitaGL, not Zeebo
 * emulation. Scene coordinates are taken directly from SCEN/VOFF geometry. */
static int cnk_draw_scene_internal(const cnk_render_mesh *mesh,const cnk_camera *camera,
                           cnk_native_texture_resolve resolve,void *context,
                           unsigned *draw_calls,uint32_t *visible_triangles,int cull){
 if(visible_triangles)*visible_triangles=0;
 if(draw_calls)*draw_calls=0;
 if(!mesh||!camera||!mesh->vertices||!mesh->batches||!mesh->vertex_count)return -1;
 /* Reject ALL malformed batches before touching GPU state: a late error
  * must never leave partially rendered geometry or client arrays enabled. */
 if(!mesh->batch_count || mesh->vertex_count%3u ||
    mesh->triangle_count!=mesh->vertex_count/3u)return -2;
 for(int i=0;i<16;i++)if(!isfinite(camera->view[i])||
                              !isfinite(camera->projection[i]))return -3;
 uint32_t cursor=0;
 for(uint32_t i=0;i<mesh->batch_count;i++){
  const cnk_render_batch *b=&mesh->batches[i];
  if(b->first_vertex!=cursor || b->vertex_count%3u ||
     b->vertex_count>mesh->vertex_count-cursor ||
     cursor>INT_MAX || b->vertex_count>INT_MAX)return -2;
  if(cull&&cnk_batch_in_frustum(camera,b)<0)return -4;
  cursor+=b->vertex_count;
 }
 if(cursor!=mesh->vertex_count)return -2;
 glViewport(0,0,960,544);
 glEnable(GL_DEPTH_TEST);
 glDepthFunc(GL_LEQUAL);
 glDisable(GL_CULL_FACE); /* winding direction in source still unverified */
 glMatrixMode(GL_PROJECTION);
 glLoadMatrixf(camera->projection);
 glMatrixMode(GL_MODELVIEW);
 glLoadMatrixf(camera->view);
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 glVertexPointer(3,GL_FLOAT,sizeof(cnk_render_vertex),&mesh->vertices[0].x);
 glTexCoordPointer(2,GL_FLOAT,sizeof(cnk_render_vertex),&mesh->vertices[0].u);
 unsigned count=0;uint32_t visible=0;
 for(uint32_t i=0;i<mesh->batch_count;i++){
  const cnk_render_batch *b=&mesh->batches[i];
  if(!b->vertex_count || (cull && !cnk_batch_in_frustum(camera,b)))continue;
  visible+=b->vertex_count/3u;
  unsigned tex=resolve?resolve(context,b->texture):0u;
  if(tex){
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D,tex);
  }else{
   /* White untextured geometry is an explicit *missing-resource marker*.
    * Do not substitute random placeholder textures in a fidelity port. */
   glDisable(GL_TEXTURE_2D);
  }
  glColor4f(1.0f,1.0f,1.0f,1.0f);
  glDrawArrays(GL_TRIANGLES,(GLint)b->first_vertex,(GLsizei)b->vertex_count);
  count++;
 }
 glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 glDisableClientState(GL_VERTEX_ARRAY);
 glDisable(GL_TEXTURE_2D);
 if(draw_calls)*draw_calls=count;
 if(visible_triangles)*visible_triangles=visible;
 return 0;
}
int cnk_vitagl_draw_scene(const cnk_render_mesh *mesh,const cnk_camera *camera,
                          cnk_native_texture_resolve resolve,void *context,
                          unsigned *draw_calls){
 return cnk_draw_scene_internal(mesh,camera,resolve,context,draw_calls,NULL,0);
}
int cnk_vitagl_draw_scene_culled(const cnk_render_mesh *mesh,const cnk_camera *camera,
                          cnk_native_texture_resolve resolve,void *context,
                          unsigned *draw_calls,uint32_t *visible_triangles){
 return cnk_draw_scene_internal(mesh,camera,resolve,context,draw_calls,visible_triangles,1);
}

