#ifndef CNK_VITA_DIAGNOSTICS_H
#define CNK_VITA_DIAGNOSTICS_H
/* Portable, low-overhead crash/error breadcrumbs for PS Vita investigation.
 * No private data, paths, assets or saved game contents are written. */
const char *cnk_vita_diagnostic_name(int code);
/* Writes a single status snapshot. A nonzero return means the diagnostic
 * could not be written, not that gameplay has a new failure. */
int cnk_vita_diagnostic_write(const char *path,int code);
#endif
