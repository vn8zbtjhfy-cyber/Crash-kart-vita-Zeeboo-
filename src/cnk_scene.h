#ifndef CNK_SCENE_H
#define CNK_SCENE_H
#include <stdint.h>
#include <stddef.h>
/* Geometría, rutas de IA y marcas de vuelta extraídas DIRECTAMENTE de SCEN/PIFF.
 * Datos de los circuitos originales; ninguna carretera o IA es inventada.
 * La escala de VERT/TCOR y la relación con VOFF aún requieren verificación
 * visual en una ejecución de referencia antes de renderizar una carrera fiel. */
typedef struct { int16_t x,y,z; } cnk_svertex;
typedef struct { int16_t u,v; } cnk_suv;
typedef struct { float x,y,z; } cnk_vec3;
typedef struct {
 uint16_t material,vertex_base,uv_base,color_base,index_width,index_count,index_word_offset;
 uint16_t *indices; /* índices LOCALES del bloque, 8/16 bit ampliados a 16 */
} cnk_schunk;
typedef struct {
 cnk_vec3 origin;
 uint16_t start,count;
 uint32_t metadata; /* significado de los bits no documentado */
} cnk_voffset;
typedef struct { char id[5]; uint32_t count; cnk_vec3 *points; } cnk_sline;
typedef struct { char id[5]; uint32_t kind; cnk_vec3 point; int32_t flags[3]; } cnk_sposition;
typedef struct {uint16_t id, flags, unknown, texture; } cnk_smaterial;
typedef struct {
 uint32_t vertex_count,uv_count,color_count,material_count,texture_count;
 uint32_t chunk_count,voffset_count,line_count,position_count;
 cnk_svertex *vertices;
 cnk_suv *uvs;
 uint16_t *colors;
 cnk_smaterial *materials;
 char **texture_names;
 cnk_schunk *chunks;
 cnk_voffset *voffsets;
 cnk_sline *lines;
 cnk_sposition *positions;
} cnk_scene;
int cnk_parse_scene(const unsigned char *data,size_t size,cnk_scene *out);
void cnk_scene_free(cnk_scene *s);
const cnk_sline *cnk_scene_line(const cnk_scene *s,const char *id);
/* Original source STRT position and 16.16 XZ forward direction from POSI.
 * Read-only metadata, NOT yet wired to the provisional AI starting grid. */
int cnk_scene_start_pose(const cnk_scene *s,cnk_vec3 *point,float *heading);
#endif
