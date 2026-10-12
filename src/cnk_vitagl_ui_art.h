#ifndef CNK_VITAGL_UI_ART_H
#define CNK_VITAGL_UI_ART_H
#include "cnk_source_ui.h"
#include <stdint.h>

/* Optional original Zeebo UI art loaded ONLY from the user's own data.vfs.
 * Handles stay in the app's GPU lifetime and are never exported or bundled. */
typedef struct {
 unsigned int handles[CNK_SOURCE_UI_COUNT];
} cnk_vitagl_ui_art;

/* -1 on any missing, invalid or GL-failing asset; all handles are rolled
 * back on failure. The caller should keep the text-based UI and continue. */
int cnk_vitagl_ui_art_load(cnk_vitagl_ui_art *out,const fufs_file *vfs);
void cnk_vitagl_ui_art_free(cnk_vitagl_ui_art *art);

/* Draw one source-backed sprite in an active 2D orthographic UI frame.
 * Sets and restores GL texture/blend/client-array state needed by itself.
 * Returns -1 if the specific art was not loaded. */
int cnk_vitagl_ui_art_draw(const cnk_vitagl_ui_art *art,unsigned kind,
                           float x,float y,float w,float h);
#endif
