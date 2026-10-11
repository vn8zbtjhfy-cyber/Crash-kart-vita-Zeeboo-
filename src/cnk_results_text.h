#ifndef CNK_RESULTS_TEXT_H
#define CNK_RESULTS_TEXT_H
#include "cnk_game.h"
/* Source-backed results presentation, not a recovered Zeebo menu/identity. */
typedef struct {
 char primary[96];
 char secondary[112];
 int local_player;
} cnk_results_line;
/* Requires CNK_MAX_RACERS slots. Returns number or <0; output is atomic. */
int cnk_results_text_build(const cnk_game_classification *classification,
                           cnk_results_line output[CNK_MAX_RACERS]);
#endif
