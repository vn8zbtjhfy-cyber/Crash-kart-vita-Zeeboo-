#ifndef CNK_STORY_CHECKPOINT_H
#define CNK_STORY_CHECKPOINT_H
#include "cnk_story_manifest.h"

/* DEV story only: portable, versioned, CRC-checked records, no raw structs.
 * Only completed, source-validated stages can be persisted. A corrupt save
 * never modifies the destination or replaces an authenticated last-good copy. */
#define CNK_STORY_CHECKPOINT_PATH_MAX 240u
int cnk_story_checkpoint_save(const cnk_story *story,const char *path);
int cnk_story_checkpoint_load(cnk_story *out,const char *path,
                              const cnk_story_manifest *manifest,
                              const cnk_track_catalog *catalog,
                              uint32_t racer_count,uint32_t player_slot);
int cnk_story_checkpoint_load_backup(cnk_story *out,const char *path,
                              const cnk_story_manifest *manifest,
                              const cnk_track_catalog *catalog,
                              uint32_t racer_count,uint32_t player_slot);
#endif
