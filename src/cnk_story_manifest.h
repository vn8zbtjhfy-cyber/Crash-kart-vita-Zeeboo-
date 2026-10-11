#ifndef CNK_STORY_MANIFEST_H
#define CNK_STORY_MANIFEST_H
#include "cnk_story.h"

typedef struct {
 uint32_t stage_count;
 cnk_story_stage stages[CNK_STORY_MAX_STAGES];
} cnk_story_manifest;
/* Developer-authored stage objectives, NOT original Zeebo Story chapters.
 * File: CNK_STORY_DEV_V1 followed by 1..12 STAGE <source-id> <laps> <max-rank>.
 * Missing file returns -2; any failure leaves *out unchanged. */
int cnk_story_manifest_read(const char *path,const cnk_track_catalog *catalog,
                            uint32_t racer_count,cnk_story_manifest *out);
#endif
