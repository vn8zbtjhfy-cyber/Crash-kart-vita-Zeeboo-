#ifndef CNK_MODEL_H
#define CNK_MODEL_H
#include <stdint.h>
#include <stddef.h>
/* Parser nativo y portátil de los objetos 3DOB/PIFF de Polarbit.
 * NO emula BREW. NO reproduce todavía animaciones, materiales ni físicas.
 * Posiciones y UV se leen tal cual del formato 16.16 del recurso original. */
typedef struct {
  float x,y,z;
  float u,v;
} cnk_vertex;
typedef struct {
  uint16_t *indices; /* Siempre índices triangulares, ya extendidos desde 8/16 bit */
  uint32_t count;
  uint32_t material;
  uint32_t original_gl_index_type;
} cnk_triangle_group;
typedef struct {
  cnk_vertex *vertices;
  uint32_t num_vertices;
  cnk_triangle_group *groups;
  uint32_t num_groups;
} cnk_model;

/* Éxito=0; fallo!=0. La salida se inicializa a cero en todos los casos.
 * La entrada debe mantenerse accesible solo durante la llamada. */
int cnk_parse_3dob(const unsigned char *data, size_t size, cnk_model *out);
void cnk_model_free(cnk_model *m);
#endif
