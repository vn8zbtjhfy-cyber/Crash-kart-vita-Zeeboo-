#include "vita_video.h"
#include <vitaGL.h>
#include <math.h>
#include <stdlib.h>
#define W 960
#define H 544
static GLuint tex=0;static uint32_t old_w=0,old_h=0;static uint32_t draw_calls=0;
// Use RGB565 directly for performance. On devices whose vitaGL build rejects
// RGB565 uploads, retry using a persistent CPU-side RGBA buffer rather than
// terminating with a black screen. Never allocate or convert in fast mode.
static int rgba_fallback=0;
static uint8_t *fallback_pixels=NULL;
static size_t fallback_capacity=0;
int vita_video_uses_rgba_fallback(void){return rgba_fallback;}
static int prepare_fallback(const uint16_t *pixels,uint32_t w,uint32_t h){
 const size_t count=(size_t)w*(size_t)h;
 if(count==0||count>2048u*2048u)return -1;
 if(count*4>fallback_capacity){
  uint8_t *next=(uint8_t*)realloc(fallback_pixels,count*4);
  if(!next)return -1;
  fallback_pixels=next;
  fallback_capacity=count*4;
 }
 for(size_t i=0;i<count;i++){
  const uint16_t v=pixels[i];
  const uint8_t r=(uint8_t)((v>>11)&31);
  const uint8_t g=(uint8_t)((v>>5)&63);
  const uint8_t b=(uint8_t)(v&31);
  fallback_pixels[4*i+0]=(uint8_t)((r<<3)|(r>>2));
  fallback_pixels[4*i+1]=(uint8_t)((g<<2)|(g>>4));
  fallback_pixels[4*i+2]=(uint8_t)((b<<3)|(b>>2));
  fallback_pixels[4*i+3]=255;
 }
 return 0;
}
static int invalid_pixel_type(GLenum err){
 return err==GL_INVALID_ENUM||err==GL_INVALID_VALUE||err==GL_INVALID_OPERATION;
}
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
int vita_video_draw(const uint16_t*r,uint32_t w,uint32_t h){
 if(!r||w==0||h==0||w>2048||h>2048)return -1;
 // Always clear the entire display, not just last frame's letterboxed viewport.
 glViewport(0,0,W,H);
 glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 {
  float s=fminf((float)W/w,(float)H/h);
  int vw=(int)(w*s+0.5f),vh=(int)(h*s+0.5f);
  glViewport((W-vw)/2,(H-vh)/2,vw,vh);glBindTexture(GL_TEXTURE_2D,tex);
  const int new_size=w!=old_w||h!=old_h;
  if(!rgba_fallback){
   if(new_size){
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,r);
   }else{
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,r);
   }
   // Check the first uploads and periodically thereafter. A rejected pixel
   // type is recoverable; VRAM exhaustion is not and remains an error.
   if(new_size||draw_calls<3||draw_calls%120==0){
    GLenum err=glGetError();
    if(err!=GL_NO_ERROR){
     if(!invalid_pixel_type(err))return (int)err;
     rgba_fallback=1;
    }
   }
  }
  if(rgba_fallback){
   if(prepare_fallback(r,w,h)!=0)return -2;
   // The first fallback upload must define a new RGBA texture, even when
   // a failed RGB565 upload already set old_w/old_h in an earlier frame.
   if(new_size||old_w==0||draw_calls<3){
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,fallback_pixels);
   }else{
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,fallback_pixels);
   }
   GLenum err=glGetError();
   if(err!=GL_NO_ERROR)return (int)err;
  }
  old_w=w;old_h=h;
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
 rgba_fallback=0;
 free(fallback_pixels);fallback_pixels=NULL;fallback_capacity=0;
 // vitaGL has no public vglEnd() API. GL allocations are owned by process lifetime.
}
