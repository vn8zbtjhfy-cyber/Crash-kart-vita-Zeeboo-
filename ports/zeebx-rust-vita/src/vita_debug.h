#pragma once
// Disk-backed boot stages. File: ux0:data/cnk_vita_boot.log
void vita_debug_reset(void);
void vita_debug_log(const char *line);
// Exposed to the Rust static library through extern "C".
void zeebx_vita_debug_log(const char *line);
