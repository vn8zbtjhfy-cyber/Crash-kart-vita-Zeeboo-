#ifndef CNK_SOURCE_UI_H
#define CNK_SOURCE_UI_H

#include "fufs.h"
#include <stddef.h>
#include <stdint.h>

/* Source-backed ORIGINAL Zeebo graphics identified by inspecting the
 * user-supplied original FUFS, not the filename-hash conjecture.
 * No copyrighted PNG data is embedded, generated, or redistributed.
 * Source UI is optional: absent/changed resources are never fabricated. */
enum cnk_source_ui_image {
 CNK_SOURCE_UI_TITLE=0,
 CNK_SOURCE_UI_WUMPA=1,
 CNK_SOURCE_UI_COUNT=2
};
typedef struct {
 uint32_t vfs_index,vfs_id,width,height,raw_png_crc32;
} cnk_source_ui_spec;

const cnk_source_ui_spec *cnk_source_ui_original(unsigned kind);

/* CRC32 of the exact, extracted PNG bytes + validated original dimensions.
 * This identifies ONE known user's original version and rejects changed
 * files safely. The check is deliberately separate for synthetic testing. */
int cnk_source_ui_verify_png(const unsigned char *data,size_t length,
                             const cnk_source_ui_spec *spec);

/* Extract an authenticated original asset from FUFS. The returned byte buffer
 * must be freed by the caller. On error neither output owns any memory. */
int cnk_source_ui_extract(const fufs_file *vfs,unsigned kind,
                          unsigned char **png,size_t *size);

/* Magenta is a colour-key background on these SOURCE UI sprite sheets.
 * Convert only exact RGB(255,0,255); preserve all other RGB and alpha.
 * If a caller passes anything other than a fully-validated RGBA buffer,
 * return an error and do not change any bytes. */
int cnk_source_ui_key_magenta(unsigned char *rgba,size_t pixels,
                              size_t byte_length,size_t *removed);
#endif
