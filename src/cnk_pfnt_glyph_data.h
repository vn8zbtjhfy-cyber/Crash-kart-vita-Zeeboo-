/* Byte-exact extraction of original PFNT encoded glyph rectangles.
 * This is NOT a decoded font bitmap: the per-font pixel format, palette,
 * alpha and scan order remain unverified.
 */
#ifndef CNK_PFNT_GLYPH_DATA_H
#define CNK_PFNT_GLYPH_DATA_H
#include "cnk_pfnt.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t codepoint;
    uint32_t atlas_x_bytes;
    uint16_t width_bytes;
    uint16_t rows;
    uint16_t atlas_row_stride_bytes;
    const uint8_t *atlas_first_byte; /* borrowed; original font must stay alive */
} cnk_pfnt_encoded_glyph;

/* Validates original font, atlas bounds and glyph lookup; 0 success.
 * Leaves *out untouched on all errors. */
int cnk_pfnt_encoded_glyph_get(const cnk_pfnt_view *font,
                               uint32_t codepoint,
                               cnk_pfnt_encoded_glyph *out);

/* Number of encoded bytes occupied by an extracted glyph. No allocation. */
size_t cnk_pfnt_encoded_glyph_bytes(const cnk_pfnt_encoded_glyph *glyph);

/* Copies each source row, preserving encoded bytes with no interpretation.
 * dst must not overlap source storage. A failed call never writes dst.
 */
int cnk_pfnt_encoded_glyph_copy(const cnk_pfnt_encoded_glyph *glyph,
                                uint8_t *dst, size_t dst_capacity);
#endif
