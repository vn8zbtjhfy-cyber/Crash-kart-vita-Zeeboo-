#pragma once
#include <stdint.h>
int vita_video_init(void);
// 0 on success; otherwise -1 for invalid guest framebuffer or an OpenGL error code.
int vita_video_draw(const uint16_t *rgb565, uint32_t width,uint32_t height);
void vita_video_shutdown(void);

// Reports when unsupported RGB565 texture uploads require the RGBA fallback.
int vita_video_uses_rgba_fallback(void);
