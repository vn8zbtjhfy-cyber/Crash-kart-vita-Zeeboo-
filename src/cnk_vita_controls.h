#ifndef CNK_VITA_CONTROLS_H
#define CNK_VITA_CONTROLS_H
#include "cnk_frontend_actions.h"
/* Provisional mappings for real PS Vita controls. Source-exact Zeebo button
 * assignments have not been recovered yet. */
int cnk_vita_controls_init(void);
int cnk_vita_controls_read(cnk_driver_input *out,int *exit_requested);
/* One physical pad sample -> race driving and edge-ready menu actions. */
int cnk_vita_controls_read_actions(cnk_driver_input *out,
                                   uint32_t *ui_buttons,
                                   int *exit_requested);
#endif
