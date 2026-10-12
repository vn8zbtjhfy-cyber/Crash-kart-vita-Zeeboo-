#pragma once
#include <stddef.h>
#include <stdint.h>
#include "embedded.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef EmbeddedAsset (*EmbeddedLookup)(const char *name);
typedef struct ZeebxVitaPad { uint32_t buttons; int32_t axes[4]; } ZeebxVitaPad;
int zeebx_vita_boot(EmbeddedLookup lookup);
int zeebx_vita_step(const ZeebxVitaPad *pad, const uint8_t **rgba, uint32_t *width, uint32_t *height);
const char *zeebx_vita_last_error(void);
void zeebx_vita_shutdown(void);
#ifdef __cplusplus
}
#endif
