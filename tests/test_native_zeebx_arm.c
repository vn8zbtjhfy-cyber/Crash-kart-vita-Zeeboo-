#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));
}
int main(void){
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome out={0};
 uint8_t module[128]={0};
 /* Synthetic RAW ARM BREW module: preserve LR, call Zeebx-style Shell
  * vtable slot 2 through an ARM BLX, then restore LR and return. */
 word(module+0, 0xe1a0400eu); /* MOV r4, lr */
 word(module+4, 0xe5903000u); /* LDR r3, [r0] -> Shell vtable */
 word(module+8, 0xe5933008u); /* LDR r3, [r3, #8] -> Shell API 2 */
 word(module+12,0xe12fff33u); /* BLX r3 */
 word(module+16,0xe1a0e004u); /* MOV lr, r4 */
 word(module+20,0xe12fff1eu); /* BX lr -> Zeebx return sentinel */
 assert(cnk_zeebx_arm_init(NULL,module,sizeof(module))<0);
 assert(cnk_zeebx_arm_init(&cpu,NULL,sizeof(module))<0);
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(cpu.regs[0]==CNK_ARM_OBJECT_BASE);
 assert(cpu.regs[1]==CNK_ARM_HELPERS_BASE);
 assert(cpu.regs[2]==CNK_ARM_OBJECT_BASE+8u);
 assert(cpu.regs[13]==CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-16u);
 assert(cpu.regs[14]==CNK_ARM_RETURN_MAGIC);
 assert(cpu.regs[15]==CNK_ARM_MOD_BASE);
 assert(cnk_zeebx_arm_run(&cpu,0,&out)<0);
 assert(!cnk_zeebx_arm_run(&cpu,2,&out));
 assert(out.reason==CNK_ARM_BUDGET&&out.executed==2);
 assert(!cnk_zeebx_arm_run(&cpu,8,&out));
 assert(out.reason==CNK_ARM_API);
 assert(out.api_interface==0u&&out.api_slot==2u);
 assert(out.pc==CNK_ARM_API_BASE+8u&&out.executed==4);
 /* Never invent an API reply: only the caller can supply a result. */
 assert(!cnk_zeebx_arm_return_from_api(&cpu,0u));
 assert(cpu.regs[0]==0u&&cpu.regs[15]==CNK_ARM_MOD_BASE+16u);
 assert(!cnk_zeebx_arm_run(&cpu,8,&out));
 assert(out.reason==CNK_ARM_RETURNED&&out.executed==6);
 cnk_zeebx_arm_destroy(&cpu);
 assert(!cpu.initialized&&!cpu.module&&!cpu.stack);

 /* Original CNK2 RAW starts with ARM B -> byte 0x14, not 0x40 BREW
  * header. This test proves we actually follow the ARM entry branch. */
 memset(module,0,sizeof(module));
 word(module,0xea000003u); /* B 0x14 */
 word(module+0x14,0xe3a0102au); /* MOV r1, #42 */
 word(module+0x18,0xe1a0f00eu); /* MOV pc, lr */
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(!cnk_zeebx_arm_run(&cpu,5,&out));
 assert(out.reason==CNK_ARM_RETURNED);
 assert(out.executed==3&&cpu.regs[1]==42u);
 cnk_zeebx_arm_destroy(&cpu);

 /* Conditional flow, stack block transfers and arithmetic, borrowed
  * semantics from Zeebx interpreter but without any game ROM. */
 memset(module,0,sizeof(module));
 word(module+ 0,0xe92d4010u); /* STMFD sp!,{r4,lr} */
 word(module+ 4,0xe3a04002u); /* MOV r4,#2 */
 word(module+ 8,0xe2544001u); /* SUBS r4,r4,#1 */
 word(module+12,0x1afffffdu); /* BNE to SUBS at offset 8 */
 word(module+16,0xe8bd4010u); /* LDMFD sp!,{r4,lr} */
 word(module+20,0xe12fff1eu); /* BX lr */
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(!cnk_zeebx_arm_run(&cpu,32,&out));
 assert(out.reason==CNK_ARM_RETURNED&&out.executed==8u);
 assert(cpu.regs[13]==CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-16u);
 cnk_zeebx_arm_destroy(&cpu);

 /* Never execute random memory, never silently accept unsupported
  * Thumb instructions or invalid/misaligned loads. */
 memset(module,0,sizeof(module));
 word(module+0,0xe3a03000u); /* MOV r3,#0 */
 word(module+4,0xe12fff13u); /* BX r3 -> unmapped guest page */
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(!cnk_zeebx_arm_run(&cpu,12,&out));
 assert(out.reason==CNK_ARM_FAULT&&out.executed==2u);
 cnk_zeebx_arm_destroy(&cpu);

 memset(module,0,sizeof(module));
 word(module+0,0xe3a00001u); /* MOV r0,#1 */
 word(module+4,0xe12fff10u); /* BX r0 -> Thumb 0 */
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(!cnk_zeebx_arm_run(&cpu,12,&out));
 assert(out.reason==CNK_ARM_FAULT || out.reason==CNK_ARM_UNSUPPORTED);
 cnk_zeebx_arm_destroy(&cpu);

 /* The heap and stack are valid DATA but never EXECUTABLE.
  * Zeebx's guest-memory permissions must survive the C adaptation. */
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 cpu.regs[15]=CNK_ARM_HEAP_BASE;
 assert(!cnk_zeebx_arm_run(&cpu,4u,&out));
 assert(out.reason==CNK_ARM_FAULT&&out.address==CNK_ARM_HEAP_BASE);
 cpu.regs[15]=CNK_ARM_STACK_BASE;
 assert(!cnk_zeebx_arm_run(&cpu,4u,&out));
 assert(out.reason==CNK_ARM_FAULT&&out.address==CNK_ARM_STACK_BASE);
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: original Zeebx ARM instruction execution, virtual BREW API traps, stack, flags, isolated failures");
 return 0;
}
