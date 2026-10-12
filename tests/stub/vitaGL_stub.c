#include "vitaGL.h"
#include <math.h>
#include <string.h>
cnk_test_gl_state cnk_test_gl;
void cnk_test_gl_reset(uint32_t n){memset(&cnk_test_gl,0,sizeof(cnk_test_gl));cnk_test_gl.vertex_limit=n;}
void glViewport(GLint x,GLint y,GLsizei w,GLsizei h){if(!((x==0&&y==0&&w==960&&h==544)||
      (x>=0&&x<=640&&x%320==0&&y==85&&w==320&&h==380)||
      (x>=0&&x<=640&&x%320==0&&(y==105||y==277)&&w==320&&h==174)))cnk_test_gl.invalid++;}
void glScissor(GLint x,GLint y,GLsizei w,GLsizei h){
 if(w<0||h<0)cnk_test_gl.invalid++;
 cnk_test_gl.scissor_calls++;
 cnk_test_gl.scissor_rect[0]=x;
 cnk_test_gl.scissor_rect[1]=y;
 cnk_test_gl.scissor_rect[2]=w;
 cnk_test_gl.scissor_rect[3]=h;
}
void glEnable(GLenum v){
 if(v==GL_TEXTURE_2D)cnk_test_gl.texture_enabled=1;
 if(v==GL_SCISSOR_TEST)cnk_test_gl.scissor_enabled=1;
}
void glDisable(GLenum v){
 if(v==GL_TEXTURE_2D)cnk_test_gl.texture_enabled=0;
 if(v==GL_SCISSOR_TEST)cnk_test_gl.scissor_enabled=0;
}
void glDepthMask(GLboolean v){if(v!=GL_TRUE&&v!=GL_FALSE)cnk_test_gl.invalid++;}
void glBlendFunc(GLenum src,GLenum dst){if(src!=GL_SRC_ALPHA||dst!=GL_ONE_MINUS_SRC_ALPHA)cnk_test_gl.invalid++;}
void glDepthFunc(GLenum v){if(v!=GL_LEQUAL)cnk_test_gl.invalid++;}
void glCullFace(GLenum v){
 if(v!=0x0404u&&v!=0x0405u&&v!=0x0408u)cnk_test_gl.invalid++;
}
void glFrontFace(GLenum v){
 if(v!=0x0900u&&v!=0x0901u)cnk_test_gl.invalid++;
}
void glColorMask(GLboolean r,GLboolean g,GLboolean b,GLboolean a){
 if((r!=GL_TRUE&&r!=GL_FALSE)||(g!=GL_TRUE&&g!=GL_FALSE)||
    (b!=GL_TRUE&&b!=GL_FALSE)||(a!=GL_TRUE&&a!=GL_FALSE))
  cnk_test_gl.invalid++;
}
void glMatrixMode(GLenum v){if(v!=GL_PROJECTION&&v!=GL_MODELVIEW)cnk_test_gl.invalid++;cnk_test_gl.matrix_mode=v;}
void glLoadMatrixf(const GLfloat *m){
 if(!m){cnk_test_gl.invalid++;return;}
 for(int i=0;i<16;i++)if(!isfinite(m[i]))cnk_test_gl.invalid++;
 cnk_test_gl.matrix_loads++;
 if(cnk_test_gl.matrix_mode==GL_PROJECTION)cnk_test_gl.projection_loads++;
 if(cnk_test_gl.matrix_mode==GL_MODELVIEW){
  cnk_test_gl.modelview_loads++;
  memcpy(cnk_test_gl.last_camera_view,m,16*sizeof(float));
 }
}
void glMultMatrixf(const GLfloat *m){
 if(!m){cnk_test_gl.invalid++;return;}
 for(int i=0;i<16;i++)if(!isfinite(m[i]))cnk_test_gl.invalid++;
 if(cnk_test_gl.matrix_multiplications<16)
   memcpy(cnk_test_gl.recorded_model_matrices+16*cnk_test_gl.matrix_multiplications,m,16*sizeof(float));
 cnk_test_gl.matrix_multiplications++;
}
void glFrustumf(GLfloat l,GLfloat r,GLfloat b,GLfloat t,GLfloat n,GLfloat f){
 if(!isfinite(l)||!isfinite(r)||!isfinite(b)||!isfinite(t)||
    !isfinite(n)||!isfinite(f)||l==r||b==t||n==f||
    n<=0.f||f<=0.f){cnk_test_gl.invalid++;return;}
 cnk_test_gl.frustum_calls++;
 const float args[6]={l,r,b,t,n,f};
 memcpy(cnk_test_gl.last_projection,args,sizeof(args));
}
void glOrthof(GLfloat l,GLfloat r,GLfloat b,GLfloat t,GLfloat n,GLfloat f){
 if(!isfinite(l)||!isfinite(r)||!isfinite(b)||!isfinite(t)||
    !isfinite(n)||!isfinite(f)||l==r||b==t||n==f){
   cnk_test_gl.invalid++;return;
 }
 cnk_test_gl.ortho_calls++;
 const float args[6]={l,r,b,t,n,f};
 memcpy(cnk_test_gl.last_projection,args,sizeof(args));
}
void glPushMatrix(void){cnk_test_gl.matrix_stack_depth++;}
void glPopMatrix(void){if(!cnk_test_gl.matrix_stack_depth)cnk_test_gl.invalid++;else cnk_test_gl.matrix_stack_depth--;}
/* Test shim validates arguments; this is not the real Vita GPU. */
void glTranslatef(GLfloat x,GLfloat y,GLfloat z){
 if(!isfinite(x)||!isfinite(y)||!isfinite(z))cnk_test_gl.invalid++;
}
void glScalef(GLfloat x,GLfloat y,GLfloat z){
 if(!isfinite(x)||!isfinite(y)||!isfinite(z))cnk_test_gl.invalid++;
}
void glRotatef(GLfloat angle,GLfloat x,GLfloat y,GLfloat z){
 if(!isfinite(angle)||!isfinite(x)||!isfinite(y)||!isfinite(z))
  cnk_test_gl.invalid++;
}

void glEnableClientState(GLenum a){(void)a;}
void glDisableClientState(GLenum a){(void)a;}
void glVertexPointer(GLint n,GLenum type,GLsizei stride,const void *p){
 if(n!=3||type!=GL_FLOAT||stride<=0||!p)cnk_test_gl.invalid++;
 cnk_test_gl.vertex_pointer=p;
}
void glTexCoordPointer(GLint n,GLenum type,GLsizei stride,const void *p){
 if(n!=2||type!=GL_FLOAT||stride<=0||!p)cnk_test_gl.invalid++;
 cnk_test_gl.uv_pointer=p;
}
void glBindTexture(GLenum type,GLuint texture){if(type!=GL_TEXTURE_2D||!texture)cnk_test_gl.invalid++;}
void glColor4f(GLfloat a,GLfloat b,GLfloat c,GLfloat d){
 if(!isfinite(a)||!isfinite(b)||!isfinite(c)||!isfinite(d)||
    a<0.f||b<0.f||c<0.f||d<0.f||a>1.f||b>1.f||c>1.f||d>1.f)
    cnk_test_gl.invalid++;
}
void glDrawArrays(GLenum mode,GLint first,GLsizei count){
 if(mode!=GL_TRIANGLES||first<0||count<=0||count%3 ||
    (uint32_t)first>cnk_test_gl.vertex_limit ||
    (uint32_t)count>cnk_test_gl.vertex_limit-(uint32_t)(first<0?0:first) ||
    !cnk_test_gl.vertex_pointer||!cnk_test_gl.uv_pointer)cnk_test_gl.invalid++;
 cnk_test_gl.draw_calls++;
 if(count>0)cnk_test_gl.drawn_vertices+=(uint32_t)count;
 if(cnk_test_gl.texture_enabled)cnk_test_gl.textured_calls++;
 else cnk_test_gl.untextured_calls++;
}

/* Host-side GL recorder for the production Vita texture loader. No emulation
 * of the original Zeebo engine; it checks upload arguments and payloads. */
static GLuint cnk_test_next_handle=1;
void glGenTextures(GLsizei n,GLuint *v){
 if(n<0||!v){cnk_test_gl.invalid++;return;}
 for(int i=0;i<n;i++){v[i]=cnk_test_next_handle++;cnk_test_gl.handles_created++;}
}
void glDeleteTextures(GLsizei n,const GLuint *v){
 if(n<0||!v){cnk_test_gl.invalid++;return;}
 for(int i=0;i<n;i++){if(!v[i])cnk_test_gl.invalid++;cnk_test_gl.handles_deleted++;}
}
void glPixelStorei(GLenum k,GLint v){if(k!=GL_UNPACK_ALIGNMENT||v!=1)cnk_test_gl.invalid++;}
void glTexParameteri(GLenum k,GLenum p,GLint v){
 if(k!=GL_TEXTURE_2D||(p!=GL_TEXTURE_MIN_FILTER&&p!=GL_TEXTURE_MAG_FILTER)||v!=GL_LINEAR)
 cnk_test_gl.invalid++;
}
void glTexEnvi(GLenum target,GLenum pname,GLint mode){
 if(target!=GL_TEXTURE_ENV||pname!=GL_TEXTURE_ENV_MODE||
    (mode!=GL_REPLACE&&mode!=GL_MODULATE&&mode!=0x2101&&
     mode!=0x0be2&&mode!=0x0104)){
  cnk_test_gl.invalid++;
  cnk_test_gl.queued_error=0x0500u; /* GL_INVALID_ENUM */
  return;
 }
 cnk_test_gl.texenv_calls++;
 cnk_test_gl.texenv_mode=(GLenum)mode;
}
void glTexImage2D(GLenum target,GLint level,GLint internalfmt,GLsizei w,
 GLsizei h,GLint border,GLenum format,GLenum type,const void *data){
 if(target!=GL_TEXTURE_2D||level!=0||w<=0||h<=0||border||!data||
    !((internalfmt==GL_RGBA&&format==GL_RGBA&&type==GL_UNSIGNED_BYTE)||
      (internalfmt==0x1907u&&format==0x1907u&&type==0x8363u))){
    cnk_test_gl.invalid++;return;}
 cnk_test_gl.images_uploaded++;
 cnk_test_gl.last_upload_width=(uint32_t)w;
 cnk_test_gl.last_upload_height=(uint32_t)h;
 cnk_test_gl.pixels_uploaded+=(uint64_t)w*(uint64_t)h;
 const unsigned char *px=(const unsigned char*)data;
 /* Inspect every decoded RGBA pixel, not only PNG metadata. */
 size_t stride=type==GL_UNSIGNED_BYTE?4u:2u;
 for(size_t i=0;i<(size_t)w*(size_t)h*stride;i+=stride){
  int nonzero=0;
  for(size_t j=0;j<stride;j++)if(px[i+j])nonzero=1;
  if(nonzero)cnk_test_gl.nonzero_texels++;
 }
 if(cnk_test_gl.fail_next_upload || (cnk_test_gl.fail_upload_on_number && cnk_test_gl.images_uploaded==cnk_test_gl.fail_upload_on_number)){
   cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
   cnk_test_gl.fail_next_upload=0;
   cnk_test_gl.fail_upload_on_number=0;
 }
}
void glTexCoord2f(GLfloat u,GLfloat v){
 if(!isfinite(u)||!isfinite(v))cnk_test_gl.invalid++;
 cnk_test_gl.texcoords_emitted++;
}
GLenum glGetError(void){GLenum e=cnk_test_gl.queued_error;cnk_test_gl.queued_error=0;return e;}
