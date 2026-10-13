// Linux regression for the same Vita input adapter used by the VPK.
// The Vita SDK header and polling API are mocked, not the game input logic.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vita_input.h"
static uint32_t guest_ms;
static SceCtrlData next_sample;
static int have_sample;
uint32_t zeebx_vita_guest_clock_ms(void){return guest_ms;}
int sceCtrlSetSamplingMode(int mode){assert(mode==SCE_CTRL_MODE_ANALOG);return 0;}
int sceCtrlPeekBufferPositive(int port, SceCtrlData *data, int max){
 assert(port==0 && max>=1);
 if(!have_sample)return 0;
 *data=next_sample;
 have_sample=0;
 return 1;
}
static ZeebxVitaPad poll_at(uint32_t clock_ms,uint64_t timestamp,uint32_t buttons){
 guest_ms=clock_ms;
 next_sample=(SceCtrlData){.timeStamp=timestamp,.buttons=buttons,.lx=128,.ly=128,.rx=128,.ry=128};
 have_sample=1;
 return vita_input_read();
}
int main(void){
 vita_input_init();
 ZeebxVitaPad a=poll_at(1000,1,SCE_CTRL_CROSS);
 assert(a.buttons&(1u<<0)); // real press
 a=poll_at(1001,2,0);
 assert(a.buttons&(1u<<0)); // release cannot erase still-unhandled tap
 a=poll_at(1119,3,0);
 assert(a.buttons&(1u<<0)); // inside 120 ms guest time
 a=poll_at(1120,4,0);
 assert(!(a.buttons&(1u<<0))); // expired, no phantom repeated taps
 a=poll_at(1200,5,SCE_CTRL_START|SCE_CTRL_SELECT);
 assert((a.buttons&(1u<<9))!=0); // Vita Start -> Zeebo HOME
 assert(vita_input_should_exit()); // physical combo always wins
 a=poll_at(1201,6,0);
 assert(!vita_input_should_exit()); // latching cannot falsely force exit
 a=poll_at(1321,7,0);
 assert(!(a.buttons&(1u<<9)));
 a=poll_at(1500,8,SCE_CTRL_RIGHT);
 assert(a.buttons&(1u<<15)); // Z-Pad D-pad right
 a=poll_at(1501,9,0);
 assert(a.buttons&(1u<<15)); // same short-tap behavior
 a=poll_at(1620,10,0);
 assert(!(a.buttons&(1u<<15)));
 puts("CNK_VITA_INPUT_TAP_TEST PASS: press, release, expiry, HOME, exit, D-pad");
 return 0;
}
