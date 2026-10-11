#ifndef CNK_MODEL_TEXL_H
#define CNK_MODEL_TEXL_H
#include <stddef.h>
#include <stdint.h>
#include "fufs.h"
/* Extract *source-declared names* from the TEXL chunks of a PIFF/3DOB.
 * Names are NOT mapped to FUFS PNG IDs by this API.
 * Strings in the source are Latin-1: verified ASCII filenames are copied;
 * non-printable/unsupported names are ignored.
 */
#define CNK_TEXL_MAX_NAMES 32u
#define CNK_TEXL_NAME_CAP 128u
typedef struct {
 char names[CNK_TEXL_MAX_NAMES][CNK_TEXL_NAME_CAP];
 unsigned count;
 unsigned opaque_tail_bytes; /* Verified uninterpreted source bytes, not filenames. */
} cnk_model_texl;
/* 0 success; negative invalid PIFF or overflowing/invalid bounds. */
int cnk_model_texl_parse(const unsigned char *bytes,size_t length,cnk_model_texl *out);
int cnk_model_texl_from_vfs(const fufs_file *vfs,uint32_t index,cnk_model_texl *out);
/* Case sensitive; requires a .png extension in all three variants.
 * When more than one matches, picks the shared name that appears earliest in
 * the most detailed source. No character attribution or file-ID inference. */
int cnk_model_texl_shared_png(const cnk_model_texl lists[3],
                              char name[CNK_TEXL_NAME_CAP]);
#endif
