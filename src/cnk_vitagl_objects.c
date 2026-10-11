#include "cnk_vitagl_objects.h"
#include "cnk_racer_visual.h"
#include <vitaGL.h>
#include <stddef.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

static int draw_object_impl(const cnk_object_mesh *mesh,
                          const cnk_camera *camera,
                          const float world_from_model[16],
                          cnk_object_material_texture resolve,void *ctx,
                          unsigned *draw_calls,uint32_t *visible_triangles,int cull){
  if(draw_calls)*draw_calls=0;
  if(!mesh||!camera||!world_from_model||!mesh->vertices||!mesh->batches ||
     !mesh->vertex_count||!mesh->batch_count)return -1;
  for(int j=0;j<16;j++)if(!isfinite(world_from_model[j]))return -2;
  /* Prevalidate the entire stream before changing GPU state. Preserve POLY
   * order because original transparency/blending policies remain unknown. */
  unsigned cursor=0;
  /* All potentially visible source batches preflight BEFORE touching GL.
   * A dynamic mask avoids assuming a limit on source material groups. */
  unsigned char small_visibility[128]={0};
  unsigned char *visible=NULL;
  int heap_visibility=0;
  if(cull){
    if(mesh->batch_count>sizeof(small_visibility)){
      visible=calloc(mesh->batch_count,sizeof(*visible));
      heap_visibility=1;
    }else visible=small_visibility;
    if(!visible)return -5;
  }
  uint32_t triangle_count=0;
  for(uint32_t i=0;i<mesh->batch_count;i++){
    const cnk_object_batch *b=&mesh->batches[i];
    if(cursor>mesh->vertex_count || b->first_vertex!=cursor || b->vertex_count%3u ||
       b->vertex_count>mesh->vertex_count-cursor ||
       cursor>INT_MAX||b->vertex_count>INT_MAX){if(heap_visibility)free(visible);return -3;}
    cursor+=b->vertex_count;
    int shown=1;
    if(cull){
      /* Reuse the exact same conservative transformed-box predicate as the
       * complete kart and driver. This also validates source batch bounds. */
      cnk_object_mesh part=*mesh;
      for(int axis=0;axis<3;axis++){
        part.bbox_min[axis]=b->bbox_min[axis];
        part.bbox_max[axis]=b->bbox_max[axis];
      }
      shown=cnk_racer_visual_in_frustum(camera,&part,world_from_model);
      if(shown<0){if(heap_visibility)free(visible);return -4;}
      visible[i]=(unsigned char)shown;
    }
    if(shown){
      if(triangle_count>UINT32_MAX-b->vertex_count/3u){if(heap_visibility)free(visible);return -6;}
      triangle_count+=b->vertex_count/3u;
    }
  }
  if(cursor!=mesh->vertex_count){if(heap_visibility)free(visible);return -3;}
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE); /* source winding has not been visually validated */
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(camera->projection);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(camera->view);
  glPushMatrix();
  glMultMatrixf(world_from_model);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3,GL_FLOAT,sizeof(cnk_render_vertex),&mesh->vertices[0].x);
  glTexCoordPointer(2,GL_FLOAT,sizeof(cnk_render_vertex),&mesh->vertices[0].u);
  unsigned calls=0;
  for(uint32_t i=0;i<mesh->batch_count;i++){
    const cnk_object_batch *b=&mesh->batches[i];
    if(!b->vertex_count || (cull && !visible[i]))continue;
    unsigned texture=resolve?resolve(ctx,b->material):0;
    if(texture){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);}
    else glDisable(GL_TEXTURE_2D); /* Unmapped materials remain visibly untextured. */
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glDrawArrays(GL_TRIANGLES,(GLint)b->first_vertex,(GLsizei)b->vertex_count);
    calls++;
  }
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
  if(draw_calls)*draw_calls=calls;
  if(visible_triangles)*visible_triangles=triangle_count;
  if(heap_visibility)free(visible);
  return 0;
}
int cnk_vitagl_draw_object(const cnk_object_mesh *mesh,
                          const cnk_camera *camera,
                          const float world_from_model[16],
                          cnk_object_material_texture resolve,void *ctx,
                          unsigned *draw_calls){
 return draw_object_impl(mesh,camera,world_from_model,resolve,ctx,draw_calls,NULL,0);
}
int cnk_vitagl_draw_object_culled(const cnk_object_mesh *mesh,
                          const cnk_camera *camera,
                          const float world_from_model[16],
                          cnk_object_material_texture resolve,void *ctx,
                          unsigned *draw_calls,uint32_t *visible_triangles){
 return draw_object_impl(mesh,camera,world_from_model,resolve,ctx,
                         draw_calls,visible_triangles,1);
}
