#ifndef CNK_CUP_CHECKPOINT_H
#define CNK_CUP_CHECKPOINT_H
#include "cnk_cup_manifest.h"
/* Explicitly developer-mode championship continuation, not Zeebo savedata.
 * Records only completed rounds, never the current unfinished race.
 * Save is atomic (.tmp followed by rename); load verifies CRC, manifest,
 * original scene IDs, rank permutations and replayed scores before mutation.
 * -2 means missing file; all failures leave *out unchanged. */
#define CNK_CUP_CHECKPOINT_PATH_MAX 240u
int cnk_cup_checkpoint_save(const cnk_cup *cup,const char *path);
/* Attempt a verified previous complete-round snapshot stored at path.bak.
 * It is never trusted without the current manifest and original track catalog. */
int cnk_cup_checkpoint_load_backup(cnk_cup *out,const char *path,
                     const cnk_cup_manifest *manifest,
                     const cnk_track_catalog *catalog);
int cnk_cup_checkpoint_load(cnk_cup *out,const char *path,
                     const cnk_cup_manifest *manifest,
                     const cnk_track_catalog *catalog);
#endif
