#ifndef CNK_RECORDS_H
#define CNK_RECORDS_H
#include <stdint.h>

/* Portable LOCAL-DEVELOPMENT records, not original Zeebo save data.
 * Versioned on-disk format, no raw struct serialization or pointer fields. */
#define CNK_RECORDS_LIMIT 128u /* 12 tracks * 4 lap options * 2 modes = 96 */
#define CNK_RECORDS_PATH_LIMIT 256u
#define CNK_RECORDS_NORMAL 0u
#define CNK_RECORDS_PRACTICE 1u
typedef struct {
 uint32_t resource_id, laps, mode;
 uint64_t best_lap_ms, best_race_ms;
} cnk_record;
typedef struct {
 uint32_t count;
 cnk_record entries[CNK_RECORDS_LIMIT];
} cnk_records;
/* Updates only improved, positive, finite-clock results. Returns 1 if changed,
 * 0 if unchanged, <0 on invalid input/capacity. A zero race time means DNF. */
int cnk_records_update(cnk_records *db,uint32_t resource_id,uint32_t laps,
                        uint32_t mode,uint64_t best_lap_ms,uint64_t race_ms);
int cnk_records_find(const cnk_records *db,uint32_t resource_id,uint32_t laps,
                      uint32_t mode,cnk_record *out);
/* Strict magic/version/checksum/length validation; load leaves db unchanged
 * on failure. Save writes temp then rename, without destroying a good file. */
int cnk_records_save(const cnk_records *db,const char *path);
int cnk_records_load(cnk_records *db,const char *path);
/* Explicit last-known-good recovery, checksum validated; no mutation on error. */
int cnk_records_load_backup(cnk_records *db,const char *path);
#endif
