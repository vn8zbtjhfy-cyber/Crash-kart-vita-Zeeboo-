#ifndef CNK_ZEEBX_PROBE_H
#define CNK_ZEEBX_PROBE_H
#include "cnk_zeebx_arm.h"

/* Optional isolated execution of user-provided ORIGINAL .mod.
 * Only validates initial ARM execution and writes a diagnostic report.
 * Never changes the existing native game state. GNU GPL-2.0-or-later. */
int cnk_zeebx_probe_original(const char *original_mod_path,
                             const char *report_path,
                             uint32_t instruction_budget,
                             cnk_arm_outcome *result);
#endif
