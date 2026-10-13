#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vitaGL.h>
#include "vita_video.h"
static GLenum next_error=GL_NO_ERROR;
static GLenum reject_error=GL_NO_ERROR;
static int images565,imagesrgba,sub565,subrgba,draws;
static uint8_t uploaded_rgba[16];
static void reset_mock(void){
 next_error=GL_NO_ERROR;reject_error=GL_NO_ERROR;
 images565=imagesrgba=sub565=subrgba=draws=0;
 memset(uploaded_rgba,0,sizeof(uploaded_rgba));
}
unsigned int vglInit(unsigned int f){(void)f;return 0;}
void vglSwapBuffers(unsigned int wait){(void)wait;}
void glViewport(int x,int y,int w,int h){(void)x;(void)y;(void)w;(void)h;}
void glDisable(GLenum cap){(void)cap;}
void glEnable(GLenum cap){(void)cap;}
void glGenTextures(int n,GLuint*t){assert(n==1);*t=7;}
void glBindTexture(GLenum target,GLuint tex){(void)target;(void)tex;}
void glTexParameteri(GLenum target,GLenum name,int value){(void)target;(void)name;(void)value;}
void glMatrixMode(GLenum mode){(void)mode;}
void glLoadIdentity(void){}
void glOrtho(double l,double r,double b,double t,double n,double f){(void)l;(void)r;(void)b;(void)t;(void)n;(void)f;}
void glClearColor(float r,float g,float b,float a){(void)r;(void)g;(void)b;(void)a;}
void glClear(GLenum mask){(void)mask;}
void glTexImage2D(GLenum target,int level,int internal,int w,int h,int border,GLenum format,GLenum type,const void*data){
 (void)target;(void)level;(void)internal;(void)border;(void)format;
 if(type==GL_UNSIGNED_SHORT_5_6_5){
  images565++;if(reject_error!=GL_NO_ERROR)next_error=reject_error;
 }else{
  assert(type==GL_UNSIGNED_BYTE&&w==2&&h==2);
  imagesrgba++;memcpy(uploaded_rgba,data,16);
 }
}
void glTexSubImage2D(GLenum target,int level,int x,int y,int w,int h,GLenum format,GLenum type,const void*data){
 (void)target;(void)level;(void)x;(void)y;(void)w;(void)h;(void)format;(void)data;
 if(type==GL_UNSIGNED_SHORT_5_6_5){
  sub565++;if(reject_error!=GL_NO_ERROR)next_error=reject_error;
 }else{
  assert(type==GL_UNSIGNED_BYTE);subrgba++;
 }
}
GLenum glGetError(void){GLenum e=next_error;next_error=GL_NO_ERROR;return e;}
void glEnableClientState(GLenum a){(void)a;}
void glDisableClientState(GLenum a){(void)a;}
void glVertexPointer(int s,GLenum t,int d,const void*p){(void)s;(void)t;(void)d;(void)p;}
void glTexCoordPointer(int s,GLenum t,int d,const void*p){(void)s;(void)t;(void)d;(void)p;}
void glDrawArrays(GLenum m,int f,int n){(void)m;(void)f;(void)n;draws++;}
void glDeleteTextures(int n,const GLuint*t){(void)n;(void)t;}
int main(void){
 const uint16_t pixels[4]={0xf800u,0x07e0u,0x001fu,0xffffu};
 reset_mock();assert(vita_video_init()==0);
 assert(vita_video_draw(pixels,2,2)==0);
 assert(images565==1&&imagesrgba==0&&!vita_video_uses_rgba_fallback());
 assert(vita_video_draw(pixels,2,2)==0);
 assert(sub565==1&&subrgba==0);vita_video_shutdown();
 reset_mock();reject_error=GL_INVALID_ENUM;
 assert(vita_video_init()==0);
 assert(vita_video_draw(pixels,2,2)==0);
 assert(vita_video_uses_rgba_fallback()&&images565==1&&imagesrgba==1);
 const uint8_t expected[16]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};
 assert(memcmp(uploaded_rgba,expected,16)==0);
 assert(vita_video_draw(pixels,2,2)==0);
 assert(subrgba==1);vita_video_shutdown();
 // A format failure AFTER the first frame must redefine the RGBA
 // texture, not attempt a subimage against a 565 texture.
 reset_mock();assert(vita_video_init()==0);
 assert(vita_video_draw(pixels,2,2)==0);
 reject_error=GL_INVALID_OPERATION;
 assert(vita_video_draw(pixels,2,2)==0);
 assert(vita_video_uses_rgba_fallback());
 assert(imagesrgba==1&&subrgba==0);vita_video_shutdown();
 reset_mock();reject_error=GL_OUT_OF_MEMORY;
 assert(vita_video_init()==0);
 assert(vita_video_draw(pixels,2,2)==(int)GL_OUT_OF_MEMORY);
 assert(!vita_video_uses_rgba_fallback());vita_video_shutdown();
 printf("CNK_VITA_GL_UPLOAD_TEST PASS: 565 fast, RGBA colors, late failure, OOM\n");
 return 0;
}
