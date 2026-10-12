#include "vita_input.h"
#include <string.h>
#define BTN(i) (1u<<(i))
static uint32_t last_buttons;
static int32_t axis(uint8_t x){return (int)x-128;}
void vita_input_init(void){sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);}
ZeebxVitaPad vita_input_read(void){
 SceCtrlData s;memset(&s,0,sizeof(s));sceCtrlPeekBufferPositive(0,&s,1);
 last_buttons=s.buttons;ZeebxVitaPad p={0};
 if(s.buttons&SCE_CTRL_CROSS)p.buttons|=BTN(0);
 if(s.buttons&SCE_CTRL_SQUARE)p.buttons|=BTN(1);
 if(s.buttons&SCE_CTRL_CIRCLE)p.buttons|=BTN(2);
 if(s.buttons&SCE_CTRL_TRIANGLE)p.buttons|=BTN(3);
 if(s.buttons&SCE_CTRL_RTRIGGER)p.buttons|=BTN(4);
 if(s.buttons&SCE_CTRL_LTRIGGER)p.buttons|=BTN(6);
 if(s.buttons&SCE_CTRL_START)p.buttons|=BTN(11);
 if(s.buttons&SCE_CTRL_UP)p.buttons|=BTN(12);
 if(s.buttons&SCE_CTRL_LEFT)p.buttons|=BTN(13);
 if(s.buttons&SCE_CTRL_DOWN)p.buttons|=BTN(14);
 if(s.buttons&SCE_CTRL_RIGHT)p.buttons|=BTN(15);
 p.axes[0]=axis(s.lx);p.axes[1]=axis(s.ly);p.axes[2]=axis(s.rx);p.axes[3]=axis(s.ry);return p;
}
int vita_input_should_exit(void){return (last_buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT);}
