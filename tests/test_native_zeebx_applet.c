/* Synthetic ARM guest program modeling Zeebx's two-stage lifecycle.
 * This is not a commercial game binary or a fabricated applet in native C.
 * Real guest ARM writes its IModule and IApplet vtable/pointers itself. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void wr32(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
int main(void){
 uint8_t code[256]={0};
 /* AEEMod_Load: malloc 36, store IModule*, assign guest vtable. */
 wr32(code+0x00,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+0x04,0xe1a05000u); /* MOV r5,r0 */
 wr32(code+0x08,0xe1a06002u); /* MOV r6,r2 */
 wr32(code+0x0c,0xe3a00024u); /* MOV r0,#36 */
 wr32(code+0x10,0xe5917068u); /* LDR r7,[r1,#0x68] malloc */
 wr32(code+0x14,0xe12fff37u); /* BLX r7 */
 wr32(code+0x18,0xe5860000u); /* STR r0,[r6] */
 wr32(code+0x1c,0xe59f301cu); /* LDR r3,[pc,#0x1c] -> literal 0x40 */
 wr32(code+0x20,0xe5803000u); /* STR r3,[r0] vtable */
 wr32(code+0x24,0xe3a00000u); /* MOV r0,#0 */
 wr32(code+0x28,0xe1a0e004u); /* MOV lr,r4 */
 wr32(code+0x2c,0xe12fff1eu); /* BX lr */
 wr32(code+0x40,CNK_ARM_MOD_BASE+0xc0u);
 /* Guest module vtable slot 2 (IModule::CreateInstance). */
 wr32(code+0xc8,CNK_ARM_MOD_BASE+0x60u);
 /* Stage 2: CreateInstance(IModule*, IShell*, ClassID, IApplet**).
  * Reuse Zeebx's helper trampoline to allocate 20 guest bytes. */
 wr32(code+0x60,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+0x64,0xe1a05003u); /* MOV r5,r3, output IApplet** */
 wr32(code+0x68,0xe3a00014u); /* MOV r0,#20 before guest malloc */
 wr32(code+0x6c,0xe59f701cu); /* LDR r7,[pc,#0x1c] helper at 0x90 */
 wr32(code+0x70,0xe12fff37u); /* BLX r7 */
 wr32(code+0x74,0xe5850000u); /* STR r0,[r5] guest writes applet pointer */
 wr32(code+0x78,0xe59f3014u); /* LDR r3,[pc,#0x14] applet vtable at 0x94 */
 wr32(code+0x7c,0xe5803000u); /* STR r3,[r0] */
 wr32(code+0x80,0xe3a00000u); /* MOV r0,#0 success */
 wr32(code+0x84,0xe1a0e004u); /* MOV lr,r4 */
 wr32(code+0x88,0xe12fff1eu); /* BX lr */
 wr32(code+0x90,CNK_ARM_API_BASE+(6u<<12)+26u*4u);
 wr32(code+0x94,CNK_ARM_MOD_BASE+0xd0u);
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome out={0};
 uint32_t calls=0,entry=0,applet=0,old=0;
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED && cpu.regs[0]==0u);
 assert(calls==1u && cpu.heap_cursor==40u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+8u,&old));
 assert(old==CNK_ARM_HEAP_BASE);
 assert(!cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry));
 assert(entry==CNK_ARM_MOD_BASE+0x60u);
 assert(cpu.regs[0]==CNK_ARM_HEAP_BASE);
 assert(cpu.regs[1]==CNK_ARM_OBJECT_BASE);
 assert(cpu.regs[2]==0x01081984u);
 assert(cpu.regs[3]==CNK_ARM_OBJECT_BASE+12u);
 assert(cpu.regs[14]==CNK_ARM_RETURN_MAGIC);
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 if(out.reason!=CNK_ARM_RETURNED || calls!=1u)
  fprintf(stderr,"stage2 status=%d pc=%08x opcode=%08x api=%u:%u calls=%u executed=%llu r0=%08x r3=%08x r5=%08x heap=%u\\n",
   (int)out.reason,out.pc,out.opcode,out.api_interface,out.api_slot,calls,
   (unsigned long long)out.executed,cpu.regs[0],cpu.regs[3],cpu.regs[5],cpu.heap_cursor);
 assert(out.reason==CNK_ARM_RETURNED && calls==1u);
 assert(cpu.heap_cursor==64u);
 assert(!cnk_zeebx_arm_applet_result(&cpu,&out,&applet));
 assert(applet==CNK_ARM_HEAP_BASE+40u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,applet,&old));
 assert(old==CNK_ARM_MOD_BASE+0xd0u);
 assert(cnk_zeebx_arm_applet_result(&cpu,&out,&applet)<0);
 cnk_zeebx_arm_destroy(&cpu);

 /* Corrupt guest module vtable pointer: it must NOT be called as ARM. */
 wr32(code+0x40,0xffffffffu);
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED);
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cpu.applet_pending);
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: Zeebx guest module/applications created by true ARM IModule::CreateInstance, safe invalid vtables");
 return 0;
}
