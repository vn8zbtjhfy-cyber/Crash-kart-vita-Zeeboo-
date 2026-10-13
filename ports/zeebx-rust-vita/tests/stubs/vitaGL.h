#pragma once
#include <stdint.h>
typedef unsigned int GLenum;
typedef unsigned int GLuint;
#define GL_TRUE 1u
#define GL_FALSE 0u
#define GL_NO_ERROR 0u
#define GL_INVALID_ENUM 0x0500u
#define GL_INVALID_VALUE 0x0501u
#define GL_INVALID_OPERATION 0x0502u
#define GL_OUT_OF_MEMORY 0x0505u
#define GL_TEXTURE_2D 0x0DE1u
#define GL_DEPTH_TEST 0x0B71u
#define GL_CULL_FACE 0x0B44u
#define GL_BLEND 0x0BE2u
#define GL_TEXTURE_MIN_FILTER 0x2801u
#define GL_TEXTURE_MAG_FILTER 0x2800u
#define GL_TEXTURE_WRAP_S 0x2802u
#define GL_TEXTURE_WRAP_T 0x2803u
#define GL_LINEAR 0x2601u
#define GL_CLAMP_TO_EDGE 0x812Fu
#define GL_PROJECTION 0x1701u
#define GL_MODELVIEW 0x1700u
#define GL_COLOR_BUFFER_BIT 0x4000u
#define GL_RGB 0x1907u
#define GL_RGBA 0x1908u
#define GL_UNSIGNED_SHORT_5_6_5 0x8363u
#define GL_UNSIGNED_BYTE 0x1401u
#define GL_VERTEX_ARRAY 0x8074u
#define GL_TEXTURE_COORD_ARRAY 0x8078u
#define GL_FLOAT 0x1406u
#define GL_TRIANGLE_STRIP 0x0005u
unsigned int vglInit(unsigned int flags);
void vglSwapBuffers(unsigned int wait);
void glViewport(int x,int y,int width,int height);
void glDisable(GLenum cap);
void glEnable(GLenum cap);
void glGenTextures(int n, GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target,GLenum name,int value);
void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glOrtho(double l,double r,double b,double t,double near,double far);
void glClearColor(float r,float g,float b,float a);
void glClear(GLenum mask);
void glTexImage2D(GLenum target,int level,int internal,int w,int h,int border,GLenum format,GLenum type,const void *data);
void glTexSubImage2D(GLenum target,int level,int x,int y,int w,int h,GLenum format,GLenum type,const void *data);
GLenum glGetError(void);
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glVertexPointer(int size,GLenum type,int stride,const void*ptr);
void glTexCoordPointer(int size,GLenum type,int stride,const void*ptr);
void glDrawArrays(GLenum mode,int first,int count);
void glDeleteTextures(int n,const GLuint *names);
