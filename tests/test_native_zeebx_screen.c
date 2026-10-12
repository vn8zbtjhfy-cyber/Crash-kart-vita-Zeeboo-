/* ROM-free ARM instruction replay of Zeebx IDisplay::GetDeviceBitmap.
 * A real guest object and vtable must be returned with stable identity.
 * Rendering methods remain explicitly unimplemented, not falsely green. */
#include "cnk_zeebx_arm.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void wr32(uint8_t *p,uint32_t v){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(v>>(8u*i));
}
int main(void){
 uint8_t code[128]={0};
 wr32(code,0xe1a0400eu);    /* MOV r4, lr */
 wr32(code+4,0xe5903000u); /* LDR r3, [r0]: display vtable */
 wr32(code+8,0xe5933040u); /* LDR r3, [r3,#64]: GetDeviceBitmap */
 wr32(code+12,0xe12fff33u);/* BLX r3 */
 wr32(code+16,0xe1a0e004u);/* MOV lr, r4 */
 wr32(code+20,0xe12fff1eu);/* BX lr */
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome o={0};
 uint32_t calls=0,addr=0,vtable=0;
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 /* Object address as issued by verified IDisplay factory. */
 cpu.objects[16]=0x00;cpu.objects[17]=0x14;
 cpu.objects[18]=0x00;cpu.objects[19]=0xe0;
 cpu.object_cursor=80u;
 cpu.regs[0]=CNK_ARM_OBJECT_BASE+16u;
 cpu.regs[1]=CNK_ARM_OBJECT_BASE+0x180u;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&o,&calls));
 assert(o.reason==CNK_ARM_RETURNED&&calls==1u);
 assert(cpu.regs[0]==0u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x180u,&addr));
 assert(addr==CNK_ARM_OBJECT_BASE+80u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,addr,&vtable));
 assert(vtable==CNK_ARM_VTABLE_BASE+7u*CNK_ARM_VTABLE_STRIDE);
 assert(cpu.screen_bitmap_addr==addr);
 assert(cpu.screen_bitmap_references==2u);
 assert(cpu.object_cursor==144u);
 /* Requesting the screen surface twice must return the SAME object. */
 cpu.regs[0]=CNK_ARM_OBJECT_BASE+16u;
 cpu.regs[1]=CNK_ARM_OBJECT_BASE+0x184u;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&o,&calls));
 assert(o.reason==CNK_ARM_RETURNED&&calls==1u);
 uint32_t other=0;
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x184u,&other));
 assert(other==addr && cpu.screen_bitmap_references==3u);
 assert(cpu.object_cursor==144u);
 /* Unknown bitmap method must still stop as an unimplemented API. */
 cpu.regs[0]=addr;
 cpu.regs[15]=CNK_ARM_API_BASE+(7u<<12)+10u*4u;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,20u,&o,&calls));
 assert(o.reason==CNK_ARM_API&&o.api_interface==7u&&o.api_slot==10u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: Zeebx ARM screen GetDeviceBitmap guest vtable, stable object, and honest draw-method trap");
 return 0;
}
