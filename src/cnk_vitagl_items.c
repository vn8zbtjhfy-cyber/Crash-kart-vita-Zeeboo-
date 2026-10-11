#include "cnk_vitagl_items.h"
#include "cnk_render.h"
#include <vitaGL.h>
#include <math.h>
#include <stddef.h>

/* Rendering is provisional billboarding, not an assertion about Zeebo sprite
 * scale, orientation, animation, blending modes or original screen layout. */
int cnk_vitagl_draw_items(const cnk_item_visual_frame *frame,const cnk_camera *camera,
                         cnk_item_sprite_resolve resolve,void *ctx,unsigned *draw_calls){
 if(!frame||!camera||!draw_calls||frame->count>CNK_ITEM_VISUAL_CAPACITY)return -1;
 for(int j=0;j<16;j++)if(!isfinite(camera->view[j])||!isfinite(camera->projection[j]))return -2;
 for(size_t i=0;i<frame->count;i++){
  const cnk_item_visual *v=&frame->actors[i];
  if(v->sprite>=CNK_ITEM_SPRITE_COUNT||v->sprite<0||
     !isfinite(v->position.x)||!isfinite(v->position.y)||!isfinite(v->position.z)||
     !isfinite(v->half_size)||v->half_size<=0.0f||v->half_size>100.0f)return -3;
 }
 unsigned count=0;
 if(!frame->count){*draw_calls=0;return 0;}
 /* Both vectors are world-space columns of the inverse rigid view rotation. */
 const cnk_vec3 right={camera->view[0],camera->view[4],camera->view[8]};
 const cnk_vec3 up={camera->view[1],camera->view[5],camera->view[9]};
 glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);
 glMatrixMode(GL_PROJECTION);glLoadMatrixf(camera->projection);
 glMatrixMode(GL_MODELVIEW);glLoadMatrixf(camera->view);
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 glDepthMask(GL_FALSE);
 static const float corners[6][2]={{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
 static const float uvs[6][2]={{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
 static const float markers[CNK_ITEM_SPRITE_COUNT][3]={
  {1.0f,0.42f,0.18f},{0.86f,0.75f,0.12f},{0.15f,0.15f,0.15f},
  {1.0f,0.45f,0.2f},{0.87f,0.18f,0.14f},{0.2f,0.45f,0.85f}
 };
 for(size_t i=0;i<frame->count;i++){
  const cnk_item_visual *v=&frame->actors[i];
  cnk_render_vertex verts[6];
  cnk_vec3 center=v->position;center.y+=v->half_size;
  for(int k=0;k<6;k++){
   float dx=corners[k][0]*v->half_size,dy=corners[k][1]*v->half_size;
   verts[k]=(cnk_render_vertex){center.x+right.x*dx+up.x*dy,
    center.y+right.y*dx+up.y*dy,center.z+right.z*dx+up.z*dy,uvs[k][0],uvs[k][1]};
  }
  unsigned texture=resolve?resolve(ctx,v->sprite):0;
  if(texture){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glColor4f(1,1,1,1);}
  else{
   glDisable(GL_TEXTURE_2D);
   glColor4f(markers[v->sprite][0],markers[v->sprite][1],markers[v->sprite][2],0.8f);
  }
  glVertexPointer(3,GL_FLOAT,sizeof(cnk_render_vertex),&verts[0].x);
  glTexCoordPointer(2,GL_FLOAT,sizeof(cnk_render_vertex),&verts[0].u);
  glDrawArrays(GL_TRIANGLES,0,6);count++;
 }
 glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_TEXTURE_2D);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY);glDisableClientState(GL_VERTEX_ARRAY);
 glColor4f(1,1,1,1);
 *draw_calls=count;return 0;
}
