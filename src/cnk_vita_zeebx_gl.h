#ifndef CNK_VITA_ZEEBX_GL_H
#define CNK_VITA_ZEEBX_GL_H
#include "cnk_zeebx_arm.h"
/* Installs a real vitaGL renderer, ONLY after vglInit() on PS Vita.
 * Used by the opt-in private-original-game probe, not by standard races.
 * Bounded DrawArrays/DrawElements XYZ triangles are forwarded to vitaGL.
 * Textures, UVs, material setup and unsupported draw modes still trap.
 */
/* Provides callbacks to the isolated original .mod probe after GPU init. */
cnk_zeebx_gl_host cnk_vita_zeebx_gl_callbacks(void);
int cnk_vita_zeebx_gl_attach(cnk_zeebx_arm *cpu);
#endif
