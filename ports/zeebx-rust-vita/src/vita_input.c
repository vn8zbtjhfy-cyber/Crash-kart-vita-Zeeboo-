#include "vita_input.h"
#include <string.h>
#include <stdint.h>
#define BTN(i) (1u<<(i))

static uint32_t last_buttons;
static uint64_t last_sample_timestamp;
static int32_t axis(uint8_t x){return (int)x-128;}

void vita_input_init(void){
 last_buttons=0;
 last_sample_timestamp=0;
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
}

ZeebxVitaPad vita_input_read(void){
 // ARM guest interpretation and software 3D rendering can stall the main
 // loop long enough to miss short presses. Peek Vita's last 64 sampled states
 // and merge ONLY the states newer than our last poll. The last physical
 // sample still controls the analog sticks and Start+Select exit.
 // We do not replay old samples: timestamp prevents phantom repeated clicks.
 SceCtrlData samples[64];
 memset(samples,0,sizeof(samples));
 const int count=sceCtrlPeekBufferPositive(0,samples,64);
 SceCtrlData latest={0};
 uint32_t fresh_buttons=0;
 uint64_t newest=last_sample_timestamp;
 int have_latest=0;
 if(count>0){
  for(int i=0;i<count;i++){
   const SceCtrlData *p=&samples[i];
   if(!have_latest || p->timeStamp>=latest.timeStamp){
    latest=*p;have_latest=1;
   }
   if(p->timeStamp>last_sample_timestamp){
    fresh_buttons |= p->buttons;
   }
   if(p->timeStamp>newest)newest=p->timeStamp;
  }
 }
 last_sample_timestamp=newest;
 last_buttons=have_latest?latest.buttons:0;
 const uint32_t buttons=last_buttons|fresh_buttons;
 ZeebxVitaPad p={0};
 if(buttons&SCE_CTRL_CROSS)p.buttons|=BTN(0);
 if(buttons&SCE_CTRL_SQUARE)p.buttons|=BTN(1);
 if(buttons&SCE_CTRL_CIRCLE)p.buttons|=BTN(2);
 if(buttons&SCE_CTRL_TRIANGLE)p.buttons|=BTN(3);
 if(buttons&SCE_CTRL_RTRIGGER)p.buttons|=BTN(4);
 if(buttons&SCE_CTRL_LTRIGGER)p.buttons|=BTN(6);
 if(buttons&SCE_CTRL_START)p.buttons|=BTN(11);
 if(buttons&SCE_CTRL_UP)p.buttons|=BTN(12);
 if(buttons&SCE_CTRL_LEFT)p.buttons|=BTN(13);
 if(buttons&SCE_CTRL_DOWN)p.buttons|=BTN(14);
 if(buttons&SCE_CTRL_RIGHT)p.buttons|=BTN(15);
 if(have_latest){
  p.axes[0]=axis(latest.lx);
  p.axes[1]=axis(latest.ly);
  p.axes[2]=axis(latest.rx);
  p.axes[3]=axis(latest.ry);
 }
 return p;
}

int vita_input_should_exit(void){
 return (last_buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT);
}
