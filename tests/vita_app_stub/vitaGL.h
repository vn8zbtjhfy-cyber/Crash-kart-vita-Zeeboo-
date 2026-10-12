#include "../stub/vitaGL.h"
#define GL_FALSE 0
#define GL_QUADS 0x0007u
#define GL_COLOR_BUFFER_BIT 0x4000u
#define GL_DEPTH_BUFFER_BIT 0x0100u
void glBegin(GLenum mode);
void glEnd(void);
void glVertex2f(float x,float y);
void glVertex3f(float x,float y,float z);
void glLoadIdentity(void);
void glPushMatrix(void);
void glOrtho(double left,double right,double bottom,double top,double near_val,double far_val);
void glClearColor(float r,float g,float b,float a);
void glClear(GLenum mask);
void vglInit(unsigned int memory);
void vglSwapBuffers(GLboolean interval);
void vglEnd(void);
