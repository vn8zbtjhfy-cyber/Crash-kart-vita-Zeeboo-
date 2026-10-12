#ifndef CNK_RACE_HUD_TEXT_H
#define CNK_RACE_HUD_TEXT_H
#include "cnk_game.h"

/* Text-only, original-source-metric HUD for a less obstructive native race.
 * Positions are based on documented Zeebo reference screenshots, not a
 * recovered original UI layout or sprite/font implementation. */
typedef struct {
 char position[24];
 char lap[24];
 char clock[32];
 char wumpa[24];
} cnk_race_hud_text;

/* Pure, deterministic and side-effect free. Does not alter race state.
 * lap in cnk_game_hud is COMPLETED laps, so the player-facing current
 * lap is completed+1, capped at total_laps. */
int cnk_race_hud_text_build(const cnk_game_hud *hud,unsigned wumpa,
                            cnk_race_hud_text *out);
#endif
