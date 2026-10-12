/* Portable synthetic recreation of the real Zeebx BREW AEEMod_Load
 * startup: helper malloc slot 26, shell AddRef slot 0, write IModule*.
 * All code bytes here are synthetic test instructions, NOT cnk2.mod. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void put(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(8u*i));
}
int main(void){
 uint8_t code[128]={0};
 /* Preserve bootstrap registers and LR before BLX calls. */
 put(code+ 0,0xe1a0400eu); /* MOV r4,lr */
 put(code+ 4,0xe1a05000u); /* MOV r5,r0  (Shell*) */
 put(code+ 8,0xe1a06002u); /* MOV r6,r2  (IModule**) */
 put(code+12,0xe3a00024u); /* MOV r0,#36  */
 put(code+16,0xe5917068u); /* LDR r7,[r1,#104] -> helper malloc 26 */
 put(code+20,0xe12fff37u); /* BLX r7 */
 put(code+24,0xe5860000u); /* STR r0,[r6] -> assigned module pointer */
 put(code+28,0xe1a00005u); /* MOV r0,r5 */
 put(code+32,0xe5903000u); /* LDR r3,[r0] */
 put(code+36,0xe5933000u); /* LDR r3,[r3] -> IShell AddRef slot 0 */
 put(code+40,0xe12fff33u); /* BLX r3 */
 put(code+44,0xe3a00000u); /* MOV r0,#0 (AEE_SUCCESS) */
 put(code+48,0xe1a0e004u); /* MOV lr,r4 */
 put(code+52,0xe12fff1eu); /* BX lr */
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome out={0};
 uint32_t apis=0,modptr=0;
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&apis));
 assert(out.reason==CNK_ARM_RETURNED);
 assert(out.executed==14u && apis==2u);
 assert(cpu.shell_refs==2u && cpu.heap_cursor==40u);
 assert(cpu.regs[0]==0);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+8u,&modptr));
 assert(modptr==CNK_ARM_HEAP_BASE);
 assert(cnk_zeebx_arm_read_u32(&cpu,0x02000000u,&modptr)<0);
 cnk_zeebx_arm_destroy(&cpu);

 /* Unknown service must be reported explicitly, NEVER a dummy success. */
 put(code+36,0xe5933008u); /* Shell::CreateInstance slot 2 */
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&apis));
 assert(out.reason==CNK_ARM_API);
 assert(out.api_interface==0u && out.api_slot==2u);
 assert(apis==1u && cpu.heap_cursor==40u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+8u,&modptr));
 assert(modptr==CNK_ARM_HEAP_BASE);
 cnk_zeebx_arm_destroy(&cpu);

 /* Limited budgets must NOT result in made-up completion. */
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,3u,&out,&apis));
 assert(out.reason==CNK_ARM_BUDGET && out.executed==3u);
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: real ARM code reaches Zeebx malloc/AddRef guest bootstrap, writes IModule pointer, unknown API stops safely");
 return 0;
}
