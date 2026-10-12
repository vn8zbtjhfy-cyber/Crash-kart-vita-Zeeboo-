/* Production Zeebx -> vitaGL adapter, linked against host-only symbol
 * recorder. These are NOT console graphics and not commercial game data.
 */
#include "cnk_vita_zeebx_gl.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static float recorded_xyz[9];
static unsigned vertices_seen,begin_calls,end_calls;
/* GL 1 immediate-mode APIs supplied by VitaGL on hardware; our host test
 * uses only these narrow functions to record the actual adapter traffic. */
void glBegin(GLenum mode){assert(mode==GL_TRIANGLES);begin_calls++;}
void glEnd(void){end_calls++;}
void glVertex3f(float x,float y,float z){
 const unsigned index=vertices_seen%3u;
 recorded_xyz[3u*index]=x;
 recorded_xyz[3u*index+1u]=y;
 recorded_xyz[3u*index+2u]=z;
 vertices_seen++;
}
void glLoadIdentity(void){}
void glClearColor(float r,float g,float b,float a){
 (void)r;(void)g;(void)b;(void)a;
}
void glClear(GLenum mask){(void)mask;}
void vglSwapBuffers(GLboolean vsync){(void)vsync;}

int main(void){
 cnk_test_gl_reset(1024u);
 cnk_zeebx_gl_host host=cnk_vita_zeebx_gl_callbacks();
 assert(host.texture_gen&&host.texture_delete&&host.texture_bind&&
        host.texture_upload&&host.texture_parameter&&
        host.draw_textured_triangles);
 uint32_t names[2]={0};
 assert(!host.texture_gen(NULL,2u,names));
 assert(names[0]&&names[1]&&names[0]!=names[1]);
 assert(cnk_test_gl.handles_created==2u);
 assert(!host.texture_bind(NULL,names[0]));
 assert(!host.texture_parameter(NULL,0x2801u,0x2601u));
 /* Crash original GL_REPLACE must reach the actual VitaGL texture state.
  * The recorder detects false success or missing GPU calls. */
 assert(host.texture_env_mode);
 assert(!host.texture_env_mode(NULL,GL_REPLACE));
 assert(cnk_test_gl.texenv_calls==1u);
 assert(cnk_test_gl.texenv_mode==GL_REPLACE);
 assert(!host.texture_env_mode(NULL,GL_MODULATE));
 assert(cnk_test_gl.texenv_calls==2u);
 assert(cnk_test_gl.texenv_mode==GL_MODULATE);
 cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
 assert(host.texture_env_mode(NULL,GL_REPLACE));
 assert(cnk_test_gl.texenv_calls==3u);
 assert(cnk_test_gl.queued_error==GL_NO_ERROR);
 uint8_t pixels[16];
 for(unsigned i=0;i<16u;i++)pixels[i]=(uint8_t)(i+1u);
 assert(!host.texture_upload(NULL,2u,2u,0x1908u,0x1401u,pixels));
 assert(cnk_test_gl.images_uploaded==1u);
 assert(cnk_test_gl.last_upload_width==2u&&
        cnk_test_gl.last_upload_height==2u);
 assert(cnk_test_gl.nonzero_texels==4u);
 uint8_t rgb565[2]={0x00u,0xf8u};
 assert(!host.texture_upload(NULL,1u,1u,0x1907u,0x8363u,rgb565));
 assert(cnk_test_gl.images_uploaded==2u);
 assert(host.texture_upload(NULL,513u,1u,0x1908u,0x1401u,pixels));

 /* The production callback forwards a real bounded atlas patch to
  * vitaGL, with GPU errors propagated instead of returning fake OK. */
 assert(host.texture_sub_upload);
 assert(!host.texture_sub_upload(NULL,0u,0u,1u,1u,
                                 0x1907u,0x8363u,rgb565));
 assert(cnk_test_gl.subimages_uploaded==1u);
 assert(cnk_test_gl.last_subimage[2]==1&&
        cnk_test_gl.last_subimage[3]==1);
 assert(host.texture_sub_upload(NULL,2u,0u,1u,1u,
                                0x1907u,0x8363u,rgb565));
 assert(cnk_test_gl.subimages_uploaded==1u);
 /* A real GPU GL_INVALID_VALUE is not hidden; this recorder counts
  * the expected invalid rectangle and clears only the TEST counter. */
 assert(cnk_test_gl.invalid==1u);
 cnk_test_gl.invalid=0u;
 cnk_test_gl.fail_next_upload=1;
 assert(host.texture_sub_upload(NULL,0u,0u,1u,1u,
                                0x1907u,0x8363u,rgb565));
 assert(cnk_test_gl.subimages_uploaded==2u);
 assert(cnk_test_gl.queued_error==GL_NO_ERROR);

 assert(cnk_test_gl.images_uploaded==2u);

 const float xyz[]={0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f};
 const float uv[]={0.f,0.f,1.f,0.f,0.f,1.f};
 assert(!host.set_capability(NULL,GL_TEXTURE_2D,1));
 assert(cnk_test_gl.texture_enabled==1);
 assert(!host.draw_textured_triangles(NULL,xyz,uv,3u));
 assert(vertices_seen==3u&&cnk_test_gl.texcoords_emitted==3u);
 assert(begin_calls==1u&&end_calls==1u);
 for(unsigned i=0;i<9u;i++)assert(recorded_xyz[i]==xyz[i]);
 assert(host.draw_textured_triangles(NULL,xyz,uv,2u));
 assert(cnk_test_gl.texcoords_emitted==3u);
 cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
 assert(host.draw_textured_triangles(NULL,xyz,uv,3u));
 assert(cnk_test_gl.texcoords_emitted==6u);
 cnk_test_gl.fail_next_upload=1;
 assert(host.texture_upload(NULL,2u,2u,0x1908u,0x1401u,pixels));
 assert(!host.set_capability(NULL,GL_TEXTURE_2D,0));
 assert(!cnk_test_gl.texture_enabled);
 assert(!host.texture_delete(NULL,2u,names));
 assert(cnk_test_gl.handles_deleted==2u);
 assert(cnk_test_gl.invalid==0u);
 puts("PASS: actual production VitaGL guest texture uploads, RGB565/RGBA,"
      " GL name lifetime, UV vertices, and surfaced GPU errors");
 return 0;
}
