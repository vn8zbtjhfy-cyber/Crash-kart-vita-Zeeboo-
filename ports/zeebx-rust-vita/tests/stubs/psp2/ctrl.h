#pragma once
#include <stdint.h>
#define SCE_CTRL_MODE_ANALOG 1
#define SCE_CTRL_CROSS    (1u<<0)
#define SCE_CTRL_SQUARE   (1u<<1)
#define SCE_CTRL_CIRCLE   (1u<<2)
#define SCE_CTRL_TRIANGLE (1u<<3)
#define SCE_CTRL_RTRIGGER (1u<<4)
#define SCE_CTRL_LTRIGGER (1u<<5)
#define SCE_CTRL_START    (1u<<6)
#define SCE_CTRL_SELECT   (1u<<7)
#define SCE_CTRL_UP       (1u<<8)
#define SCE_CTRL_LEFT     (1u<<9)
#define SCE_CTRL_DOWN     (1u<<10)
#define SCE_CTRL_RIGHT    (1u<<11)
typedef struct SceCtrlData {
 uint64_t timeStamp;
 uint32_t buttons;
 uint8_t lx,ly,rx,ry;
} SceCtrlData;
int sceCtrlSetSamplingMode(int mode);
int sceCtrlPeekBufferPositive(int port, SceCtrlData *data, int count);
