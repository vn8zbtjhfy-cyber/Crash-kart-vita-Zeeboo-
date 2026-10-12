#ifndef CNK_ZEEBX_BREW_H
#define CNK_ZEEBX_BREW_H
#include <stddef.h>
#include <stdint.h>

/* ZeebxTeam's BREW .mod/.mif format reader adapted from Rust to C99.
 * Upstream GPL-2.0-or-later; see third_party/zeebx/LICENSE, NOTICE.md.
 * INSPECTION/IDENTITY only: does NOT execute guest code or emulate BREW.
 * Neither the original .mod nor .mif is included in the source tree.
 */
#define CNK_BREW_MAX_SECTIONS 256u
#define CNK_BREW_MAX_CLASSES 64u
typedef struct {
 uint32_t main_applet;
 uint32_t applet_count;
 uint32_t class_count;
 uint32_t classes[CNK_BREW_MAX_CLASSES];
 uint32_t sections;
 uint32_t first_section_offset;
 uint32_t last_section_end;
} cnk_brew_mif_info;

typedef struct {
 uint32_t entry_offset;
 uint32_t first_branch_target;
 uint32_t code_offset;
 uint32_t brew_version;
 uint32_t first_instruction;
 uint32_t header_words[10];
 uint32_t image_bytes;
 int has_brew_header;
 int has_initial_branch;
} cnk_brew_mod_info;

/* All parsing is read-only, strictly bounded, atomic on failure.
 * .mod entry_offset is the CPU start address for BREW execution (0 for
 * raw modules). first_branch_target captures its first ARM B instruction,
 * when any. Neither is a native Vita entrypoint. */
int cnk_brew_mod_inspect(const uint8_t *file,size_t size,cnk_brew_mod_info *out);
int cnk_brew_mif_inspect(const uint8_t *file,size_t size,cnk_brew_mif_info *out);
#endif
