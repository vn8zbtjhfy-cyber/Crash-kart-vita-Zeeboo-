#ifndef CNK_FRONTEND_ACTIONS_H
#define CNK_FRONTEND_ACTIONS_H
#include "cnk_frontend.h"
#include <stdint.h>

/* Abstract edge-triggered native controls. Original Zeebo menu button
 * assignments are not yet recovered. PS Vita mapping is adapter-side. */
enum {
 CNK_UI_PREVIOUS=1u<<0,
 CNK_UI_NEXT=1u<<1,
 CNK_UI_CONFIRM=1u<<2,
 CNK_UI_CANCEL=1u<<3,
 CNK_UI_PAUSE=1u<<4,
 CNK_UI_RESTART=1u<<5,
 CNK_UI_DRIFT=1u<<6,
 CNK_UI_ITEM=1u<<7,
 CNK_UI_RECOVER=1u<<8,
 CNK_UI_LAPS=1u<<9, /* SQUARE in race selector only */
 CNK_UI_LANGUAGE=1u<<10 /* SELECT: handled by Vita menu adapter only */
};
#define CNK_UI_ALL ((1u<<10)-1u)
typedef struct {uint32_t last_buttons;} cnk_frontend_action_state;
/* Return physics substeps, 0 on an action/no-op, or negative on error.
 * A held button never repeats automatically, and a menu action never
 * leaks a large time delta into the newly loaded race. */
int cnk_frontend_dispatch(cnk_frontend *ui,cnk_frontend_action_state *state,
                          uint32_t held_buttons,cnk_driver_input driving,
                          uint32_t dt_us);
#endif
