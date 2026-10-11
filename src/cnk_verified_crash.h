#ifndef CNK_VERIFIED_CRASH_H
#define CNK_VERIFIED_CRASH_H
#include "cnk_object_skin.h"
#include <stdint.h>
/* One source-supported character asset pair, not a generic filename hash.
 * Original 3DOB #19 material Crash.png; original PNG #365 ee898141.
 * Load only after vitaGL has initialized. No vehicle identity, skeleton,
 * animation, kart placement or engine behavior is inferred here. */
typedef struct {
 cnk_object_skin original;
} cnk_verified_crash;
int cnk_verified_crash_load(const fufs_file *vfs,
                            const cnk_png_catalog *catalog,cnk_verified_crash *out);
unsigned int cnk_verified_crash_resolve(void *ctx,uint32_t original_material);
void cnk_verified_crash_free(cnk_verified_crash *character);
#endif
