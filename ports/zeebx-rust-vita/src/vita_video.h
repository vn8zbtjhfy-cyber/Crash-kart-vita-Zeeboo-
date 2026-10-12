#pragma once
#include <stdint.h>
int vita_video_init(void);
void vita_video_draw(const uint8_t *rgba, uint32_t width,uint32_t height);
void vita_video_shutdown(void);
