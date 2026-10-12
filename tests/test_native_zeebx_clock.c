/* Zeebx BREW virtual time (helper indices 43/44), synthetic ARM only.
 * The guest must see elapsed milliseconds provided by its scheduler,
 * never the host clock or a fabricated per-instruction timestamp. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void wr32(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(i*8u));
}
int main(void){
 uint8_t code[128]={0};
 wr32(code+0,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+4,0xe59130acu); /* LDR r3,[r1,#172] aee_GetTimeMS */
 wr32(code+8,0xe12fff33u); /* BLX r3 */
 wr32(code+12,0xe1a05000u);/* MOV r5,r0 */
 wr32(code+16,0xe59130b0u);/* LDR r3,[r1,#176] aee_GetUpTimeMS */
 wr32(code+20,0xe12fff33u);/* BLX r3 */
 wr32(code+24,0xe1a0e004u);/* MOV lr,r4 */
 wr32(code+28,0xe12fff1eu);/* BX lr */
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome stop={0};
 uint32_t calls=0;
 assert(cnk_zeebx_arm_set_virtual_time(&cpu,12u)<0);
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(cpu.virtual_time_ms==0u);
 assert(!cnk_zeebx_arm_set_virtual_time(&cpu,1250u));
 assert(cnk_zeebx_arm_set_virtual_time(&cpu,1249u)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,100u,&stop,&calls));
 assert(stop.reason==CNK_ARM_RETURNED);
 assert(stop.executed==8u && calls==2u);
 assert(cpu.regs[5]==1250u && cpu.regs[0]==1250u);
 assert(!cnk_zeebx_arm_set_virtual_time(&cpu,1251u));
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,100u,&stop,&calls));
 assert(stop.reason==CNK_ARM_RETURNED);
 assert(calls==2u && cpu.regs[0]==1251u && cpu.regs[5]==1251u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: Zeebx real ARM AEE GetTimeMS and GetUpTimeMS respect explicit monotonic virtual clock");
 return 0;
}
