/* Minimal graphics translation of original Zeebo Common-Lite commands
 * to the actual PS Vita vitaGL renderer, not a software success stub.
 * GPL-2.0-or-later adaptation of Zeebx legacy EGL/GL ABI.
 * The caller MUST have called vglInit() before attaching this sink.
 * The original 640x480 4:3 image is centered at 725x544 on 960x544.
 */
#include "cnk_vita_zeebx_gl.h"
#include <vitaGL.h>
#include <stdint.h>
#include <limits.h>
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x0400u
#endif

#define CNK_NATIVE_DISPLAY_W 960
#define CNK_NATIVE_DISPLAY_H 544
#define CNK_ZEEBO_DISPLAY_W 640
#define CNK_ZEEBO_DISPLAY_H 480
#define CNK_PORT_WIDTH 725
#define CNK_PORT_X ((CNK_NATIVE_DISPLAY_W-CNK_PORT_WIDTH)/2)

static int vita_begin(void *user,uint32_t w,uint32_t h){
 (void)user;
 if(w!=CNK_ZEEBO_DISPLAY_W||h!=CNK_ZEEBO_DISPLAY_H)return -1;
 glViewport(CNK_PORT_X,0,CNK_PORT_WIDTH,CNK_NATIVE_DISPLAY_H);
 return 0;
}
static int vita_clear_color(void *user,float r,float g,float b,float a){
 (void)user;
 glClearColor(r,g,b,a);
 return 0;
}
static int vita_clear(void *user,uint32_t mask){
 (void)user;
 if(mask&~(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT))
  return -1;
 glClear((GLenum)mask);
 return 0;
}
static int vita_viewport(void *user,int32_t x,int32_t y,
                         uint32_t w,uint32_t h){
 (void)user;
 /* Maintain original Zeebo pixel coordinate space and 4:3 aspect.
  * Large values are rejected before conversion to GL signed ints. */
 if(w>4096u||h>4096u||x>4096||x< -4096||
    y>4096||y< -4096)return -1;
 const int64_t vx=CNK_PORT_X+((int64_t)x*CNK_PORT_WIDTH)/640;
 const int64_t vy=((int64_t)y*CNK_NATIVE_DISPLAY_H)/480;
 const int64_t vw=((int64_t)w*CNK_PORT_WIDTH)/640;
 const int64_t vh=((int64_t)h*CNK_NATIVE_DISPLAY_H)/480;
 if(vx<INT_MIN||vx>INT_MAX||vy<INT_MIN||vy>INT_MAX||
    vw>INT_MAX||vh>INT_MAX)return -1;
 glViewport((GLint)vx,(GLint)vy,(GLsizei)vw,(GLsizei)vh);
 return 0;
}
static int vita_present(void *user){
 (void)user;
 vglSwapBuffers(GL_FALSE);
 return 0;
}
/* OpenGL ES 1.x matrix stack operations: submit to the initialized Vita
 * graphics context. GL errors (including stack underflow) trap the guest. */
static int vita_matrix_mode(void *user,uint32_t mode){
 (void)user;
 glMatrixMode((GLenum)mode);
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_load_identity(void *user){
 (void)user;
 glLoadIdentity();
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_push_matrix(void *user){
 (void)user;
 glPushMatrix();
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_pop_matrix(void *user){
 (void)user;
 glPopMatrix();
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_translate(void *user,float x,float y,float z){
 (void)user;
 glTranslatef(x,y,z);
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_scale(void *user,float x,float y,float z){
 (void)user;
 glScalef(x,y,z);
 return glGetError()==GL_NO_ERROR?0:-1;
}
static int vita_rotate(void *user,float degrees,float x,float y,float z){
 (void)user;
 glRotatef(degrees,x,y,z);
 return glGetError()==GL_NO_ERROR?0:-1;
}
/* Each vertex is immediately submitted as part of a real vitaGL triangle
 * stream. No transient guest pointer can outlive this call; GPU errors
 * stop original guest execution rather than claim a successful frame. */
static int vita_draw_triangles(void *user,const float *xyz,uint32_t count){
 (void)user;
 if(!xyz||!count||count>CNK_ZEEBX_MAX_DRAW_VERTICES||count%3u)
  return -1;
 glBegin(GL_TRIANGLES);
 for(uint32_t i=0;i<count;i++)
  glVertex3f(xyz[3u*i],xyz[3u*i+1u],xyz[3u*i+2u]);
 glEnd();
 return glGetError()==GL_NO_ERROR?0:-1;
}
cnk_zeebx_gl_host cnk_vita_zeebx_gl_callbacks(void){
 const cnk_zeebx_gl_host host={
  .make_current=vita_begin,.clear_color=vita_clear_color,
  .clear=vita_clear,.viewport=vita_viewport,.present=vita_present,
  .matrix_mode=vita_matrix_mode,.load_identity=vita_load_identity,
  .push_matrix=vita_push_matrix,.pop_matrix=vita_pop_matrix,
  .translate=vita_translate,.scale=vita_scale,.rotate=vita_rotate,
  .draw_triangles=vita_draw_triangles
 };
 return host;
}
int cnk_vita_zeebx_gl_attach(cnk_zeebx_arm *cpu){
 const cnk_zeebx_gl_host host=cnk_vita_zeebx_gl_callbacks();
 return cnk_zeebx_arm_set_gl_host(cpu,&host,NULL);
}
