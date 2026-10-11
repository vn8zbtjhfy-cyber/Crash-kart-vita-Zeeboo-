#ifndef CNK_CUP_MANIFEST_H
#define CNK_CUP_MANIFEST_H
#include "cnk_cup.h"

/* Developer-authored data, NOT a recovered original Zeebo cup listing.
 * The reader accepts exactly one marker, 2..12 TRACK rows and one POINTS
 * row containing a strictly descending score per racer. Source resource
 * IDs are checked against the authenticated ROM scene catalog. */
typedef struct {
 uint32_t round_count;
 uint32_t racer_count;
 uint32_t resource_ids[CNK_CUP_MAX_ROUNDS];
 uint32_t points[CNK_MAX_RACERS];
} cnk_cup_manifest;
/* Missing file returns -2. Every failure leaves *out unchanged.
 * No default circuits or scores are synthesized. */
int cnk_cup_manifest_read(const char *path,const cnk_track_catalog *catalog,
                          uint32_t racer_count,cnk_cup_manifest *out);
#endif
