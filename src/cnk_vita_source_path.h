#ifndef CNK_VITA_SOURCE_PATH_H
#define CNK_VITA_SOURCE_PATH_H
#include <stddef.h>

/* Locate a user-provided FUFS archive without distributing source assets.
 * Returns one of the supplied string pointers, or NULL if none appears to
 * contain a FUFS header. The full archive is validated later by fufs_open(). */
const char *cnk_vita_source_select(const char *const *candidates,size_t count);

/* Preferred external location for the original user-owned data.vfs:
 * ux0:data/cnk3d/data.vfs. Legacy paths are accepted for diagnostics. */
const char *cnk_vita_source_find(void);
#endif
