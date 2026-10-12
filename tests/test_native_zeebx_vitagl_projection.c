/* Production PS Vita Zeebx renderer with a host-only vitaGL recorder.
 * This proves callback wiring but is NOT physical hardware validation.
 */
#include "cnk_vita_zeebx_gl.h"
#include "vitaGL.h"
#include <assert.h>
#include <stdio.h>

/* Required by unrelated existing draw/present callbacks: no hardware GPU. */
void glBegin(GLenum mode){assert(mode==GL_TRIANGLES);}
void glEnd(void){}
void glLoadIdentity(void){}
void glVertex3f(float x,float y,float z){(void)x;(void)y;(void)z;}
void glClearColor(float r,float g,float b,float a){
 (void)r;(void)g;(void)b;(void)a;
}
void glClear(GLenum mask){(void)mask;}
void vglSwapBuffers(GLboolean vsync){(void)vsync;}

int main(void){
 cnk_test_gl_reset(2048u);
 cnk_zeebx_gl_host gpu=cnk_vita_zeebx_gl_callbacks();
 assert(gpu.load_matrix&&gpu.mult_matrix&&gpu.frustum&&gpu.ortho);
 /* The exact original Zeebo 640x480 rectangle maps into the Vita
  * 725x544 centered pillarbox. No stretching to 960x544. */
 assert(gpu.scissor&&gpu.set_capability);
 assert(!gpu.set_capability(NULL,GL_SCISSOR_TEST,1));
 assert(cnk_test_gl.scissor_enabled==1);
 assert(!gpu.scissor(NULL,0,0,640u,480u));
 assert(cnk_test_gl.scissor_calls==1u);
 assert(cnk_test_gl.scissor_rect[0]==117 &&
        cnk_test_gl.scissor_rect[1]==0 &&
        cnk_test_gl.scissor_rect[2]==725 &&
        cnk_test_gl.scissor_rect[3]==544);
 assert(!gpu.scissor(NULL,160,120,320u,240u));
 assert(cnk_test_gl.scissor_calls==2u);
 assert(cnk_test_gl.scissor_rect[0]==298 &&
        cnk_test_gl.scissor_rect[1]==136 &&
        cnk_test_gl.scissor_rect[2]==362 &&
        cnk_test_gl.scissor_rect[3]==272);
 assert(gpu.scissor(NULL,0,0,5000u,240u)!=0);
 assert(cnk_test_gl.scissor_calls==2u);
 cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
 assert(gpu.scissor(NULL,0,0,640u,480u)!=0);
 assert(cnk_test_gl.scissor_calls==3u);
 assert(!gpu.set_capability(NULL,GL_SCISSOR_TEST,0));
 assert(cnk_test_gl.scissor_enabled==0);
 assert(gpu.matrix_mode);
 assert(!gpu.matrix_mode(NULL,GL_PROJECTION));
 const float source[16]={
  2.f,0.f,0.f,0.f,0.f,-1.5f,0.f,0.f,
  0.f,0.f,1.f,0.f,0.5f,0.f,0.f,1.f
 };
 assert(!gpu.load_matrix(NULL,source));
 assert(cnk_test_gl.matrix_loads==1u);
 assert(cnk_test_gl.projection_loads==1u);
 assert(gpu.load_matrix(NULL,NULL)!=0);
 assert(!gpu.mult_matrix(NULL,source));
 assert(cnk_test_gl.matrix_multiplications==1u);
 for(unsigned i=0;i<16u;i++)
  assert(cnk_test_gl.recorded_model_matrices[i]==source[i]);
 assert(gpu.mult_matrix(NULL,NULL)!=0);
 assert(!gpu.frustum(NULL,-1.f,1.f,-0.5f,0.5f,1.f,16.f));
 assert(cnk_test_gl.frustum_calls==1u);
 assert(cnk_test_gl.last_projection[0]==-1.f);
 assert(cnk_test_gl.last_projection[4]==1.f);
 assert(cnk_test_gl.last_projection[5]==16.f);
 assert(gpu.frustum(NULL,-1.f,1.f,-0.5f,0.5f,0.f,16.f)!=0);
 assert(cnk_test_gl.frustum_calls==1u);
 assert(!gpu.ortho(NULL,0.f,640.f,0.f,480.f,-1.f,1.f));
 assert(cnk_test_gl.ortho_calls==1u);
 assert(cnk_test_gl.last_projection[1]==640.f);
 assert(cnk_test_gl.last_projection[3]==480.f);
 assert(cnk_test_gl.last_projection[4]==-1.f);
 assert(gpu.ortho(NULL,0.f,0.f,0.f,480.f,-1.f,1.f)!=0);
 assert(cnk_test_gl.ortho_calls==1u);
 cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
 assert(gpu.frustum(NULL,-1.f,1.f,-0.5f,0.5f,1.f,16.f)!=0);
 assert(cnk_test_gl.frustum_calls==2u); /* forwarded, GL error reported */
 assert(cnk_test_gl.invalid==0u);
 puts("PASS: production Zeebx vitaGL projection/matrix callbacks deliver "
      "actual GL operations and never hide a host GL error");
 return 0;
}
