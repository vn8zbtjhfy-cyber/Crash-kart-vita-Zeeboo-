/* PFNT character map and advance metrics recovered from source Zeebo data.vfs.
 * This does not decode the glyph bitmap or assign historical font roles.
 * View borrows data: keep its input buffer alive until no longer needed.
 */
#ifndef CNK_PFNT_H
#define CNK_PFNT_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *bytes;
  size_t length;
  uint16_t codepoint_first, codepoint_last, glyph_count;
  uint8_t line_height_px;
  uint32_t total_advance_px;
  size_t trailing_payload_offset;
} cnk_pfnt_view;

typedef struct {
  uint16_t codepoint;
  uint32_t atlas_x_px;
  uint8_t advance_px;
} cnk_pfnt_glyph;

/* Returns 0 if all index structures are self-consistent, negative otherwise. */
int cnk_pfnt_open(const uint8_t *data, size_t size, cnk_pfnt_view *out);
/* 0 = found, 1 = absent, negative = invalid argument/view. */
int cnk_pfnt_glyph_for(const cnk_pfnt_view *font, uint32_t codepoint,
                        cnk_pfnt_glyph *out);
/* UTF-8 to advances. Returns -3 for unsupported characters, -4 for malformed
 * UTF-8. Does not fabricate a missing-glyph width or spacing/kerning. */
int cnk_pfnt_measure_utf8(const cnk_pfnt_view *font, const char *text,
                           size_t text_bytes, uint32_t *width_px);

/* AVANCE 85: Evidenced PFNT backing pixel byte stream. This describes a
 * verified byte range, NOT decoded RGBA / glyph masks. PFNT color/index
 * interpretation and pixel scan order are still unverified.
 * Each view borrows the PFNT original bytes.
 */
typedef struct {
  const uint8_t *encoded_pixels;
  size_t encoded_bytes;
  const uint8_t *image_preamble;
  size_t preamble_bytes;
  uint16_t stored_width_px;
  uint16_t stored_height_px;
} cnk_pfnt_encoded_atlas;

/* Finds a precisely-sized encoded payload at the end of the PFNT resource;
 * fails closed on truncated/inconsistent input. */
int cnk_pfnt_encoded_atlas_open(const cnk_pfnt_view *font,
                                cnk_pfnt_encoded_atlas *out);

/* Positions authentic PFNT advances in a UI layout without inventing glyph
 * bitmaps. Newline is layout control; all other codepoints must be in PFNT.
 * No outputs (including the output array) change on failure.
 */
typedef struct {
  uint32_t codepoint;
  uint32_t source_x_px;
  uint32_t dest_x_px;
  uint32_t dest_y_px;
  uint8_t advance_px;
} cnk_pfnt_positioned_glyph;

int cnk_pfnt_layout_utf8(const cnk_pfnt_view *font,
                          const char *text, size_t text_bytes,
                          cnk_pfnt_positioned_glyph *glyphs,
                          size_t glyph_capacity,
                          size_t *glyph_count,
                          uint32_t *text_width_px,
                          uint32_t *text_height_px);

#endif /* CNK_PFNT_H */
