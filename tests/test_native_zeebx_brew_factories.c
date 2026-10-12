/* Verified Zeebx BREW classes seen in user's PRIVATE original cnk2.mod.
 * ROM-free original synthetic guest ARM instructions and pointers.
 * Does not claim that IDisplay or IFileMgr methods are implemented. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void wr32(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(i*8u));
}
int main(void){
 uint8_t guest[256]={0};
 /* Guest ARM calls IShell::CreateInstance via the real Shell vtable. */
 wr32(guest+0,0xe1a0400eu);  /* MOV r4,lr */
 wr32(guest+4,0xe1a05002u);  /* MOV r5,r2 */
 wr32(guest+8,0xe5903000u);  /* LDR r3,[r0]  */
 wr32(guest+12,0xe5933008u); /* LDR r3,[r3,#8] Shell slot 2 */
 wr32(guest+16,0xe12fff33u); /* BLX r3 */
 wr32(guest+20,0xe1a0e004u); /* MOV lr,r4 */
 wr32(guest+24,0xe12fff1eu); /* BX lr */
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome outcome={0};
 uint32_t handled=0,ptr=0,vtbl=0,slot=0;
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=0x01001001u; /* AEECLSID_DISPLAY observed in real CNK */
 cpu.regs[2]=CNK_ARM_OBJECT_BASE+0x100u;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,80u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.regs[0]==0u && cpu.display_objects==1u &&
        cpu.filemgr_objects==0u);
 assert(cpu.object_cursor==80u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x100u,&ptr));
 assert(ptr==CNK_ARM_OBJECT_BASE+16u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,ptr,&vtbl));
 assert(vtbl==CNK_ARM_VTABLE_BASE+5u*CNK_ARM_VTABLE_STRIDE);
 assert(!cnk_zeebx_arm_read_u32(&cpu,vtbl+4u*4u,&slot));
 assert(slot==CNK_ARM_API_BASE+(5u<<12)+4u*4u);
 /* A real guest call can request FileMgr subsequently. */
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=0x01001003u; /* AEECLSID_FILEMGR */
 cpu.regs[2]=CNK_ARM_OBJECT_BASE+0x104u;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,80u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.display_objects==1u&&cpu.filemgr_objects==1u);
 assert(cpu.object_cursor==144u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x104u,&ptr));
 assert(ptr==CNK_ARM_OBJECT_BASE+80u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,ptr,&vtbl));
 assert(vtbl==CNK_ARM_VTABLE_BASE+3u*CNK_ARM_VTABLE_STRIDE);
 assert(!cnk_zeebx_arm_read_u32(&cpu,vtbl+2u*4u,&slot));
 assert(slot==CNK_ARM_API_BASE+(3u<<12)+2u*4u);
 /* Unsupported ClassID remains a visible trap, never a fake object. */
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=0x0100f00du;
 cpu.regs[2]=CNK_ARM_OBJECT_BASE+0x108u;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,80u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_API);
 assert(outcome.api_interface==0u&&outcome.api_slot==2u);
 assert(cpu.object_cursor==144u&&handled==0u);
 cnk_zeebx_arm_destroy(&cpu);

 /* The original startup calls dbgprintf; Zeebx returns SUCCESS.
  * Its guest string is treated as DATA, not as a host format string. */
 memset(guest,0,sizeof(guest));
 wr32(guest+0,0xe1a0400eu); /* MOV r4,lr */
 wr32(guest+4,0xe591309cu); /* LDR r3,[r1,#156] helper slot 39 */
 wr32(guest+8,0xe12fff33u); /* BLX r3 */
 wr32(guest+12,0xe1a0e004u);/* MOV lr,r4 */
 wr32(guest+16,0xe12fff1eu);/* BX lr */
 memcpy(guest+0x50,"AEEApplet_New: %s",18u);
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 cpu.regs[0]=CNK_ARM_MOD_BASE+0x50u;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.debug_calls==1u);
 assert(!strcmp(cpu.last_debug_template,"AEEApplet_New: %s"));
 assert(cpu.regs[0]==0u);
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: genuine Zeebx IShell Display/FileMgr classes produce guest objects and dbgprintf is safely captured");
 return 0;
}
