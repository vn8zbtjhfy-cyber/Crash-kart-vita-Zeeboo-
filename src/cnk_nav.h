#ifndef CNK_NAV_H
#define CNK_NAV_H
#include "cnk_scene.h"
/* Operaciones matemáticas sobre las rutas DRV originales del juego.
 * El algoritmo exacto de conducción BREW todavía NO se ha decompilado.
 * Estas operaciones preservan la posición de los puntos de la ROM,
 * no sustituyen la física ni la IA original. */
typedef struct { cnk_vec3 point; float fraction, distance; uint32_t segment; } cnk_nav_projection;
float cnk_nav_length(const cnk_sline *line, int closed);
int cnk_nav_sample(const cnk_sline *line, float fraction, int closed, cnk_vec3 *result);
/* Same sampling as cnk_nav_sample, using a length cached from IMMUTABLE
 * source DRV0. Saves a full route-length traversal per AI sample. */
int cnk_nav_sample_with_length(const cnk_sline *line,float fraction,int closed,
                               float route_length,cnk_vec3 *result);
int cnk_nav_nearest(const cnk_sline *line, cnk_vec3 position, int closed, cnk_nav_projection *result);
/* Devuelve el signo de cruce de una línea LAP original en el plano XZ.
 * -1,0,+1; el signo correcto para validar una vuelta se investiga después. */
int cnk_nav_gate_crossing(const cnk_sline *lap,cnk_vec3 before,cnk_vec3 after);
#endif
