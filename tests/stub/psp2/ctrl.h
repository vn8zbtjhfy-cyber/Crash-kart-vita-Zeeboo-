#ifndef CNK_TEST_CTRL_H
#define CNK_TEST_CTRL_H
#include <stdint.h>
#define SCE_CTRL_MODE_ANALOG 1
#define SCE_CTRL_LEFT 0x0001u
#define SCE_CTRL_RIGHT 0x0002u
#define SCE_CTRL_CROSS 0x0004u
#define SCE_CTRL_SQUARE 0x0008u
#define SCE_CTRL_CIRCLE 0x0010u
#define SCE_CTRL_START 0x0020u
#define SCE_CTRL_SELECT 0x0040u
#define SCE_CTRL_TRIANGLE 0x0080u
#define SCE_CTRL_RTRIGGER 0x0100u
#define SCE_CTRL_LTRIGGER 0x0200u
#define SCE_CTRL_UP 0x0400u
#define SCE_CTRL_DOWN 0x0800u
typedef struct {uint32_t buttons;uint8_t lx,ly,rx,ry;} SceCtrlData;
int sceCtrlSetSamplingMode(int mode);
int sceCtrlPeekBufferPositive(int port,SceCtrlData *pad,int count);
#endif
