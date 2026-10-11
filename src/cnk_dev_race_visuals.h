#ifndef CNK_DEV_RACE_VISUALS_H
#define CNK_DEV_RACE_VISUALS_H
#include "cnk_racer_visual.h"
#include "cnk_source_kart_catalog.h"
#include "cnk_session.h"
#include "fufs.h"

/* DEVELOPMENT-ONLY visual fallback. These six meshes are authentic Zeebo
 * resources, but NONE of the kart-to-driver, orientation, scale or racing
 * slot assignments have been established as historical originals. Never
 * show as provisional fallback in incomplete development builds. A verified
 * racer binding always takes precedence; a diagnostic opt-out is supported. */
#define CNK_DEV_VISUAL_FAMILIES 6u
typedef struct {
 cnk_object_mesh meshes[CNK_DEV_VISUAL_FAMILIES];
 float local[CNK_DEV_VISUAL_FAMILIES][16];
 int ready;
} cnk_dev_race_visuals;

/* Atomic: a bad ID, mesh or bounding box produces an empty result. */
int cnk_dev_race_visuals_load(const fufs_file *rom, cnk_dev_race_visuals *out);
void cnk_dev_race_visuals_free(cnk_dev_race_visuals *visuals);
/* Fill ONLY missing bindings. Verified user-supplied associations win. */
int cnk_dev_race_visuals_fill(const cnk_dev_race_visuals *visuals,
 unsigned racer_count, cnk_racer_visual_attachment out[CNK_MAX_RACERS]);
#endif
