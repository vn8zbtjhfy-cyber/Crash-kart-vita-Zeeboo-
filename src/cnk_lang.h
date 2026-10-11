#ifndef CNK_LANG_H
#define CNK_LANG_H
#include "fufs.h"
#include <stdint.h>
/* Original LANG resource from the user-provided Zeebo data.vfs.
 * Strings are indexed by their ORIGINAL per-language ordinal, not guessed IDs.
 * This module does not infer the source game's UI layout or font rendering. */
#define CNK_LANG_LOCALES 5u
#define CNK_LANG_STRINGS 229u
#define CNK_LANG_SOURCE_INDEX 48u
#define CNK_LANG_SOURCE_ID 0x1b50648au

typedef enum {
 CNK_LANG_ENGLISH=0, CNK_LANG_FRENCH=1, CNK_LANG_ITALIAN=2,
 CNK_LANG_PORTUGUESE=3, CNK_LANG_SPANISH=4
} cnk_lang_locale;
typedef struct {
 uint32_t locale;
 uint32_t count;
 char name[32]; /* original language name converted from source UTF-16LE */
 char *storage; /* owns a single UTF-8 allocation for all source strings */
 const char *text[CNK_LANG_STRINGS];
} cnk_lang_pack;
/* Loads ONLY the chosen original language, with source-ID and layout checks.
 * On failure leaves *out untouched. Caller zero-initializes and frees old packs.
 * Never executes original Zeebo code and never modifies source assets. */
int cnk_lang_load(const fufs_file *f,cnk_lang_locale locale,cnk_lang_pack *out);
/* Optional plain one-digit configuration, 0-4 (+ optional CR/LF).
 * Missing file: -2; invalid: negative. *out never changes on failure. */
int cnk_lang_preference_read(const char *path,cnk_lang_locale *out);
/* Best-effort atomic preference save: temporary file + rename, no ROM writes. */
int cnk_lang_preference_write(const char *path,cnk_lang_locale locale);
/* Switch to the next ORIGINAL language. On failure preserve the old pack. */
int cnk_lang_cycle(const fufs_file *f,cnk_lang_pack *current);
void cnk_lang_free(cnk_lang_pack *lang);
const char *cnk_lang_text(const cnk_lang_pack *lang,uint32_t original_ordinal);
#endif
