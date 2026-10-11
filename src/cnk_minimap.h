#ifndef CNK_MINIMAP_H
#define CNK_MINIMAP_H
#include "cnk_session.h"
/* 2D developmental HUD built only from the current original SCEN DRV0 path;
 * no invented track shape or source artwork. Normalized XZ coords in [0,1]. */
#define CNK_MINIMAP_PATH_POINTS 96u
typedef struct {float x,z;} cnk_minimap_point;
typedef struct {
 cnk_minimap_point path[CNK_MINIMAP_PATH_POINTS];
 cnk_minimap_point racers[CNK_MAX_RACERS];
 uint32_t path_count,racer_count;
 /* Route transform cache, recomputed only when a new source scene loads. */
 float minx,minz,scale,offx,offz;
} cnk_minimap_frame;
int cnk_minimap_build(const cnk_session *session,cnk_minimap_frame *out);
int cnk_minimap_update_racers(const cnk_session *session,cnk_minimap_frame *map);
#endif
