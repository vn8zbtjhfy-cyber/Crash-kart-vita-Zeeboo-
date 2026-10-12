#include "cnk_zeebx_probe.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void wr(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8u));
}
int main(void){
 const char *source="cnk_zeebx_probe_synthetic.mod"; /* host temporary ONLY */
 const char *report="cnk_zeebx_probe_synthetic.txt";
 uint8_t code[128]={0};
 wr(code+0, 0xe1a0400eu);  /* MOV r4,lr */
 wr(code+4, 0xe5903000u);  /* LDR r3,[r0] */
 wr(code+8, 0xe5933008u);  /* LDR r3,[r3,#8] */
 wr(code+12,0xe12fff33u);  /* BLX r3 */
 cnk_arm_outcome outcome={0};
 assert(cnk_zeebx_probe_original(source,report,16,&outcome)<0);
 assert(outcome.executed==0);
 FILE *f=fopen(source,"wb");assert(f);
 assert(fwrite(code,1,sizeof(code),f)==sizeof(code));
 assert(!fclose(f));
 assert(!cnk_zeebx_probe_original(source,report,16,&outcome));
 assert(outcome.reason==CNK_ARM_API&&outcome.api_slot==2u);
 assert(outcome.executed==4u);
 f=fopen(report,"rb");assert(f);
 char buffer[500]={0};
 size_t n=fread(buffer,1,sizeof(buffer)-1u,f);
 assert(!fclose(f));
 buffer[n]='\0';
 assert(strstr(buffer,"reason=BREW_API"));
 assert(strstr(buffer,"api_interface=0"));
 assert(strstr(buffer,"api_slot=2"));
 assert(strstr(buffer,"genuine_game_playable=NO"));
 assert(!remove(source));
 assert(!remove(report));
 puts("PASS: opt-in original-code probe reports first BREW trap without touching native racing game");
 return 0;
}
