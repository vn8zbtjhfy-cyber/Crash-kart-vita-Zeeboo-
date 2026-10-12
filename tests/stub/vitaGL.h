/* Test-only vitaGL symbol shim, NOT a renderer, not part of the Vita build.
 * Compiles the production bridge without simulating the game itself.
 */
#ifndef CNK_TEST_VITAGL_H
#define CNK_TEST_VITAGL_H
#include <stdint.h>
typedef unsigned int GLenum;
typedef int GLboolean;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
#define GL_DEPTH_TEST 0x0B71u
#define GL_COLOR_BUFFER_BIT 0x4000u
#define GL_DEPTH_BUFFER_BIT 0x0100u
#define GL_STENCIL_BUFFER_BIT 0x0400u
#define GL_BLEND 0x0BE2u
#define GL_SCISSOR_TEST 0x0C11u
#define GL_SRC_ALPHA 0x0302u
#define GL_ONE_MINUS_SRC_ALPHA 0x0303u
#define GL_TRUE 1
#define GL_FALSE 0
#define GL_LEQUAL 0x0203u
#define GL_CULL_FACE 0x0B44u
#define GL_PROJECTION 0x1701u
#define GL_MODELVIEW 0x1700u
#define GL_VERTEX_ARRAY 0x8074u
#define GL_TEXTURE_COORD_ARRAY 0x8078u
#define GL_FLOAT 0x1406u
#define GL_TEXTURE_2D 0x0DE1u
#define GL_TEXTURE_ENV 0x2300u
#define GL_TEXTURE_ENV_MODE 0x2200u
#define GL_REPLACE 0x1E01u
#define GL_MODULATE 0x2100u
#define GL_TRIANGLES 0x0004u
#define GL_RGBA 0x1908u
#define GL_UNSIGNED_BYTE 0x1401u
#define GL_UNPACK_ALIGNMENT 0x0CF5u
#define GL_TEXTURE_MIN_FILTER 0x2801u
#define GL_TEXTURE_MAG_FILTER 0x2800u
#define GL_LINEAR 0x2601u
#define GL_NO_ERROR 0u
#define GL_OUT_OF_MEMORY 0x0505u
/* Narrow declarations used by the REAL Vita Zeebx graphics adapter.
 * Test executables supply host-only recorders, never the console GPU. */
void glBegin(GLenum primitive);
void glEnd(void);
void glVertex3f(GLfloat x,GLfloat y,GLfloat z);
void glClearColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a);
void glClear(GLenum mask);
void glLoadIdentity(void);
void vglSwapBuffers(GLboolean vsync);
void glGenTextures(GLsizei n,GLuint *textures);
void glDeleteTextures(GLsizei n,const GLuint *textures);
void glPixelStorei(GLenum pname,GLint param);
void glTexParameteri(GLenum target,GLenum pname,GLint param);
void glTexEnvi(GLenum target,GLenum pname,GLint param);
void glTexImage2D(GLenum target,GLint level,GLint internalformat,GLsizei width,
                  GLsizei height,GLint border,GLenum format,GLenum type,
                  const void *data);
GLenum glGetError(void);
void glViewport(GLint x,GLint y,GLsizei width,GLsizei height);
void glScissor(GLint x,GLint y,GLsizei width,GLsizei height);
void glEnable(GLenum capability);
void glDisable(GLenum capability);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean enabled);
void glBlendFunc(GLenum src,GLenum dst);
void glCullFace(GLenum face);
void glFrontFace(GLenum mode);
void glColorMask(GLboolean red,GLboolean green,GLboolean blue,GLboolean alpha);
void glMatrixMode(GLenum mode);
void glLoadMatrixf(const GLfloat *matrix);
void glMultMatrixf(const GLfloat *matrix);
void glFrustumf(GLfloat l,GLfloat r,GLfloat b,GLfloat t,GLfloat n,GLfloat f);
void glOrthof(GLfloat l,GLfloat r,GLfloat b,GLfloat t,GLfloat n,GLfloat f);
void glPushMatrix(void);
void glPopMatrix(void);
void glTranslatef(GLfloat x,GLfloat y,GLfloat z);
void glScalef(GLfloat x,GLfloat y,GLfloat z);
void glRotatef(GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glVertexPointer(GLint size,GLenum type,GLsizei stride,const void *ptr);
void glTexCoordPointer(GLint size,GLenum type,GLsizei stride,const void *ptr);
void glBindTexture(GLenum target,GLuint texture);
void glColor4f(GLfloat r,GLfloat g,GLfloat b,GLfloat a);
void glDrawArrays(GLenum mode,GLint first,GLsizei count);
void glTexCoord2f(GLfloat u,GLfloat v);

typedef struct {
 uint32_t vertex_limit;
 uint64_t drawn_vertices, draw_calls, textured_calls, untextured_calls;
 uint32_t texcoords_emitted;
 uint32_t scissor_calls;
 GLint scissor_rect[4];
 int scissor_enabled;
 uint32_t matrix_loads,matrix_multiplications, matrix_stack_depth;
 float recorded_model_matrices[16 * 16];
 float last_camera_view[16];
 uint32_t projection_loads,modelview_loads, invalid;
 uint32_t frustum_calls,ortho_calls;
 float last_projection[6];
 uint32_t texture_callbacks;
 uint32_t texenv_calls;
 GLenum texenv_mode;
 uint32_t images_uploaded,handles_created,handles_deleted;
 uint64_t pixels_uploaded, nonzero_texels;
 uint32_t last_upload_width,last_upload_height;
 GLenum queued_error;
 int fail_next_upload;
 uint32_t fail_upload_on_number;
 int texture_enabled;
 GLenum matrix_mode;
 const void *vertex_pointer,*uv_pointer;
} cnk_test_gl_state;
extern cnk_test_gl_state cnk_test_gl;
void cnk_test_gl_reset(uint32_t vertex_limit);
#endif
