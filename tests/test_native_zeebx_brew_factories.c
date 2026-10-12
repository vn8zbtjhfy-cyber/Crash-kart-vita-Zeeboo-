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
 /* CNK's actual next ClassID after DeviceInfo: ISound (Zeebx iface 14).
  * The factory is valid, but unknown sound methods must still trap. */
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=0x01001056u;
 cpu.regs[2]=CNK_ARM_OBJECT_BASE+0x10cu;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,80u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.sound_objects==1u&&cpu.object_cursor==208u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x10cu,&ptr));
 assert(ptr==CNK_ARM_OBJECT_BASE+144u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,ptr,&vtbl));
 assert(vtbl==CNK_ARM_VTABLE_BASE+14u*CNK_ARM_VTABLE_STRIDE);
 assert(!cnk_zeebx_arm_read_u32(&cpu,vtbl+4u*4u,&slot));
 assert(slot==CNK_ARM_API_BASE+(14u<<12)+4u*4u);
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

 /* Real guest call to ISHELL_GetDeviceInfo, Zeebx slot 4.
  * Extended fields are written ONLY when wStructSize requests 64. */
 memset(guest,0,sizeof(guest));
 wr32(guest+0,0xe1a0400eu);  /* MOV r4,lr */
 wr32(guest+4,0xe5903000u);  /* LDR r3,[r0] */
 wr32(guest+8,0xe5933010u);  /* LDR r3,[r3,#16] */
 wr32(guest+12,0xe12fff33u); /* BLX r3 */
 wr32(guest+16,0xe1a0e004u); /* MOV lr,r4 */
 wr32(guest+20,0xe12fff1eu); /* BX lr */
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 cpu.objects[0x200u+44u]=64u;
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=CNK_ARM_OBJECT_BASE+0x200u;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.regs[0]==0u);
 assert(cpu.objects[0x200u]==0x80u&&cpu.objects[0x201u]==0x02u); /* 640 */
 assert(cpu.objects[0x202u]==0xe0u&&cpu.objects[0x203u]==0x01u);/* 480 */
 assert(cpu.objects[0x208u]==8u&&cpu.objects[0x20au]==3u);
 assert(cpu.objects[0x20eu]==16u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+0x200u+24u,&slot));
 assert(slot==64u*1024u*1024u);
 assert(cpu.objects[0x200u+44u]==64u&&cpu.objects[0x200u+56u]==64u);
 /* A short struct must not overwrite bytes 44..63. */
 cpu.objects[0x200u+44u]=0u;
 cpu.objects[0x200u+56u]=0x7bu;
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=CNK_ARM_OBJECT_BASE+0x200u;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED);
 assert(cpu.objects[0x200u+44u]==0u&&cpu.objects[0x200u+56u]==0x7bu);
 /* Out-of-bounds AEEDeviceInfo is a visible API stop, not data overwrite. */
 cpu.regs[0]=CNK_ARM_OBJECT_BASE;
 cpu.regs[1]=CNK_ARM_OBJECT_BASE+0xff0u;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_API&&outcome.api_slot==4u);
 cnk_zeebx_arm_destroy(&cpu);

 /* ARM calls Zeebx's AEEStdLib::memset at helper slot 1.
  * Check exact guest heap bytes, unchanged sentinels and return pointer. */
 memset(guest,0,sizeof(guest));
 wr32(guest+0,0xe1a0400eu);  /* MOV r4,lr */
 wr32(guest+4,0xe5937004u);  /* LDR r7,[r3,#4] */
 wr32(guest+8,0xe12fff37u);  /* BLX r7 */
 wr32(guest+12,0xe1a0e004u); /* MOV lr,r4 */
 wr32(guest+16,0xe12fff1eu); /* BX lr */
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 cpu.regs[0]=CNK_ARM_HEAP_BASE+8u;
 cpu.regs[1]=0x1234abu;
 cpu.regs[2]=32u;
 cpu.regs[3]=CNK_ARM_HELPERS_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(cpu.regs[0]==CNK_ARM_HEAP_BASE+8u);
 assert(cpu.heap[7u]==0u&&cpu.heap[40u]==0u);
 for(unsigned i=8u;i<40u;i++)assert(cpu.heap[i]==0xabu);
 /* Bad destination must remain stopped at the exact API. */
 cpu.regs[0]=CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-8u;
 cpu.regs[1]=0xffu;
 cpu.regs[2]=32u;
 cpu.regs[3]=CNK_ARM_HELPERS_BASE;
 cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_MOD_BASE;
 assert(!cnk_zeebx_arm_bootstrap(&cpu,30u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_API&&outcome.api_interface==6u&&outcome.api_slot==1u);
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: Zeebx guest Display/FileMgr/ISound, real DeviceInfo layout, memset, dbgprintf and unknown API stops");
 return 0;
}
