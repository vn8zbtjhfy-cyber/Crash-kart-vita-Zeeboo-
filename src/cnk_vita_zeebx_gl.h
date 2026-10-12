#ifndef CNK_VITA_ZEEBX_GL_H
#define CNK_VITA_ZEEBX_GL_H
#include "cnk_zeebx_arm.h"
/* Installs a real vitaGL renderer, ONLY after vglInit() on PS Vita.
 * Used by the opt-in private-original-game probe, not by standard races.
 * DrawArrays/Elements and textures remain unsupported and trap.
 */
int cnk_vita_zeebx_gl_attach(cnk_zeebx_arm *cpu);
#endif
