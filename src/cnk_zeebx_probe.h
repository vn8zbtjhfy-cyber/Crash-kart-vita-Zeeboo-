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

/* Full opt-in diagnostic: derives applet ClassID from user's own .mif,
 * then calls the REAL guest IModule::CreateInstance vtable when the
 * module load has succeeded. Fails visibly on unsupported BREW/Thumb.
 * The existing native racing runtime is never modified. */
int cnk_zeebx_probe_original_applet(const char *original_mod_path,
                                    const char *original_mif_path,
                                    const char *report_path,
                                    uint32_t instruction_budget,
                                    cnk_arm_outcome *result);
/* An opt-in launch-time snapshot of physical controller input for the
 * original guest bootstrap. This is not yet per-frame HID polling.
 * The module and applet are still executed only through a bounded probe. */
int cnk_zeebx_probe_original_applet_with_pad(const char *original_mod_path,
                                              const char *original_mif_path,
                                              const char *report_path,
                                              uint32_t instruction_budget,
                                              const cnk_zeebx_pad *pad,
                                              cnk_arm_outcome *result);
/* GPU-enabled diagnostic is allowed only when a real host vitaGL context
 * already exists. Without bound callbacks, guest GL still traps.
 * Original commercial game files remain private and off-repository. */
int cnk_zeebx_probe_original_applet_with_gl(
 const char *original_mod_path,const char *original_mif_path,
 const char *report_path,uint32_t instruction_budget,
 const cnk_zeebx_pad *pad,const cnk_zeebx_gl_host *host,
 void *host_user,cnk_arm_outcome *result);
#endif
