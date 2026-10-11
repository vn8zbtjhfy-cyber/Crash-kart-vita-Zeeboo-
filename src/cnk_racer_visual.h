#ifndef CNK_RACER_VISUAL_H
#define CNK_RACER_VISUAL_H
#include "cnk_drive.h"
#include "cnk_object_render.h"
#include "cnk_camera.h"
#include <stdint.h>

/* A SOURCE-VERIFIED model selection and local calibration are provided by the
 * application, NOT guessed by this module. Passing a mesh does NOT prove it
 * belongs to Crash or to any particular kart. No fallback mesh is fabricated.
 * Column-major local_from_mesh can include source-confirmed scale/orientation.
 */
typedef struct {
    const cnk_object_mesh *mesh; /* NULL deliberately leaves a racer invisible */
    const float *local_from_mesh; /* sixteen column-major floats, required with mesh */
    unsigned int (*resolve_material)(void *ctx,uint32_t material);
    void *material_context;
} cnk_racer_visual_attachment;

/* Compose kart translation + yaw with a calibrated local-model transform.
 * Kart forward 0 rad = +Z; +yaw steers toward +X, Y remains upright.
 * Fails before modifying output for bad/NaN values.
 */
int cnk_racer_visual_matrix(const cnk_kart *kart,
                            const float local_from_mesh[16],
                            float out_world_from_mesh[16]);
/* Validate a whole binding before issuing GPU calls. */
int cnk_racer_visual_attachment_valid(const cnk_racer_visual_attachment *v);
/* Conservative SOURCE-MESH bounds after calibrated kart+driver transform.
 * 1=possibly visible, 0=definitely offscreen, negative=invalid geometry.
 * Never assumes model dimensions, road width or unverified vehicle identity. */
int cnk_racer_visual_in_frustum(const cnk_camera *camera,
             const cnk_object_mesh *mesh,const float world_from_mesh[16]);
#endif
