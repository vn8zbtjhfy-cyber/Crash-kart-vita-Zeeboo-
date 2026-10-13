#include "vita_video.h"
#include <vitaGL.h>
#include <math.h>
#define W 960
#define H 544
static GLuint tex=0;static uint32_t old_w=0,old_h=0;static uint32_t draw_calls=0;
int vita_video_init(void){
 // vitaGL's vglInit() returns GL_TRUE only when requested resolution
 // was clamped to display limits; GL_FALSE is also returned on normal
 // successful initialization. It is NOT a conventional success boolean.
 // See upstream vitaGL source/vgl.c: return res_fallback.
 (void)vglInit(0);
 glViewport(0,0,W,H);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
 glDisable(GL_BLEND);glEnable(GL_TEXTURE_2D);
 glGenTextures(1,&tex);
 if(tex==0)return -1; // A missing GPU texture cannot display any guest frame.
 glBindTexture(GL_TEXTURE_2D,tex);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(-1,1,-1,1,-1,1);
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 // Force the first real GPU scene and swap immediately after vitaGL init.
 // vitaGL terminates its animated splash on the first draw; do not leave
 // that splash on-screen while Zeebx initializes the game.
 glViewport(0,0,W,H);
 glClearColor(0,0,0,1);
 glClear(GL_COLOR_BUFFER_BIT);
 vglSwapBuffers(GL_FALSE);
 return 0;
}
int vita_video_draw(const uint8_t*r,uint32_t w,uint32_t h){
 if(!r||w==0||h==0||w>2048||h>2048)return -1;
 // Always clear the entire display, not just last frame's letterboxed viewport.
 glViewport(0,0,W,H);
 glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 {
  float s=fminf((float)W/w,(float)H/h);
  int vw=(int)(w*s+0.5f),vh=(int)(h*s+0.5f);
  glViewport((W-vw)/2,(H-vh)/2,vw,vh);glBindTexture(GL_TEXTURE_2D,tex);
  if(w!=old_w||h!=old_h){
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,r);
   // Detect VRAM allocation failures at the moment of texture creation
   // instead of silently showing black or triggering a later GPU fault.
   GLenum err=glGetError();
   if(err!=GL_NO_ERROR)return (int)err;
   old_w=w;old_h=h;
  }else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,r);
  const float v[]={-1,-1,1,-1,-1,1,1,1};
  const float t[]={0,1,1,1,0,0,1,0};
  glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2,GL_FLOAT,0,v);glTexCoordPointer(2,GL_FLOAT,0,t);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);glDisableClientState(GL_VERTEX_ARRAY);
 }
 // A sparse check also detects deferred texture-update/draw failures
 // without synchronizing GL error state on every rendered game frame.
 if((++draw_calls%120)==0){
  GLenum err=glGetError();
  if(err!=GL_NO_ERROR)return (int)err;
 }
 vglSwapBuffers(GL_FALSE);
 return 0;
}
void vita_video_shutdown(void){
 if(tex){glDeleteTextures(1,&tex);tex=0;}
 old_w=0;old_h=0;draw_calls=0;
 // vitaGL has no public vglEnd() API. GL allocations are owned by process lifetime.
}
