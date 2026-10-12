/* Zeebx original BREW Shell timer calls and callbacks. Synthetic ARM
 * only: no Crash ROM, sound, music or assets are included in CI.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void put(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(8u*i));
}
static uint32_t guest_word(cnk_zeebx_arm *cpu,uint32_t addr){
 uint32_t value=0u;assert(!cnk_zeebx_arm_read_u32(cpu,addr,&value));
 return value;
}
static void invoke(cnk_zeebx_arm *cpu,unsigned slot,
                   uint32_t a1,uint32_t a2,uint32_t a3){
 cpu->regs[0]=CNK_ARM_OBJECT_BASE;
 cpu->regs[1]=a1;cpu->regs[2]=a2;cpu->regs[3]=a3;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_API_BASE+slot*4u;
}
static uint32_t successful_api(cnk_zeebx_arm *cpu){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,20u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED && handled==1u);
 return cpu->regs[0];
}
int main(void){
 uint8_t guest[192]={0};
 /* void timer_callback(void* context) { ++*(uint32_t*)context; } */
 put(guest+64u,0xe5901000u); /* LDR r1,[r0] */
 put(guest+68u,0xe2811001u); /* ADD r1,r1,#1 */
 put(guest+72u,0xe5801000u); /* STR r1,[r0] */
 put(guest+76u,0xe12fff1eu); /* BX lr */
 const uint32_t fn=CNK_ARM_MOD_BASE+64u;
 const uint32_t ctx=CNK_ARM_HEAP_BASE+0x100u;
 cnk_zeebx_arm c={0};cnk_arm_outcome result={0};
 uint32_t delivered=99u;
 assert(!cnk_zeebx_arm_init(&c,guest,sizeof(guest)));
 assert(!cnk_zeebx_arm_set_virtual_time(&c,100u));
 invoke(&c,11u,20u,fn,ctx); /* IShell::SetTimer */
 assert(successful_api(&c)==0u);
 assert(c.timer_count==1u && c.timers[0].deadline_ms==120u);
 invoke(&c,13u,fn,ctx,0u); /* IShell::GetTimerExpiration */
 assert(successful_api(&c)==20u);
 assert(!cnk_zeebx_arm_set_virtual_time(&c,110u));
 invoke(&c,13u,fn,ctx,0u);
 assert(successful_api(&c)==10u);
 assert(!cnk_zeebx_arm_deliver_timers(&c,64u,&result,&delivered));
 assert(result.reason==CNK_ARM_RETURNED && delivered==0u);
 assert(c.timer_count==1u && guest_word(&c,ctx)==0u);
 assert(!cnk_zeebx_arm_set_virtual_time(&c,120u));
 uint32_t saved_regs[16];
 memcpy(saved_regs,c.regs,sizeof(saved_regs));
 const uint32_t saved_flags=c.cpsr;
 const uint64_t before=c.executed;
 assert(!cnk_zeebx_arm_deliver_timers(&c,64u,&result,&delivered));
 assert(result.reason==CNK_ARM_RETURNED && delivered==1u);
 assert(c.timer_count==0u && guest_word(&c,ctx)==1u);
 assert(c.executed==before+4u);
 assert(!memcmp(c.regs,saved_regs,sizeof(saved_regs)) && c.cpsr==saved_flags);
 invoke(&c,13u,fn,ctx,0u);
 assert(successful_api(&c)==0u); /* removed after callback */

 /* Zeebx re-arms SAME function+context, replacing, not duplicating. */
 invoke(&c,11u,50u,fn,ctx);
 assert(successful_api(&c)==0u);
 invoke(&c,11u,15u,fn,ctx);
 assert(successful_api(&c)==0u);
 assert(c.timer_count==1u&&c.timers[0].deadline_ms==135u);
 invoke(&c,13u,fn,ctx,0u);
 assert(successful_api(&c)==15u);
 invoke(&c,12u,fn,ctx,0u); /* CancelTimer */
 assert(successful_api(&c)==0u && !c.timer_count);
 assert(!cnk_zeebx_arm_set_virtual_time(&c,200u));
 assert(!cnk_zeebx_arm_deliver_timers(&c,64u,&result,&delivered));
 assert(delivered==0u && guest_word(&c,ctx)==1u);

 /* Negative int32 wait is due immediately, not 49 days later. */
 invoke(&c,11u,(uint32_t)-2,fn,ctx);
 assert(successful_api(&c)==0u);
 assert(c.timer_count==1u && c.timers[0].deadline_ms==200u);
 assert(!cnk_zeebx_arm_deliver_timers(&c,64u,&result,&delivered));
 assert(delivered==1u && guest_word(&c,ctx)==2u);

 /* CancelTimer(NULL,ctx) cancels ALL callbacks for this user. */
 invoke(&c,11u,0u,fn,ctx);
 assert(successful_api(&c)==0u);
 invoke(&c,11u,0u,fn+4u,ctx);
 assert(successful_api(&c)==0u);
 invoke(&c,12u,0u,ctx,0u);
 assert(successful_api(&c)==0u && !c.timer_count);
 /* Null timer function rejects as BREW_API without changing state. */
 invoke(&c,11u,0u,0u,ctx);
 uint32_t calls=0u;
 assert(!cnk_zeebx_arm_bootstrap(&c,20u,&result,&calls));
 assert(result.reason==CNK_ARM_API && result.api_slot==11u);
 assert(c.timer_count==0u);
 assert(!cnk_zeebx_arm_return_from_api(&c,1u));
 assert(c.regs[15]==CNK_ARM_RETURN_MAGIC);

 /* Wrong object pointer must never impersonate the BREW Shell. */
 invoke(&c,11u,1u,fn,ctx);
 c.regs[0]=ctx;
 assert(!cnk_zeebx_arm_bootstrap(&c,20u,&result,&calls));
 assert(result.reason==CNK_ARM_API && c.timer_count==0u);
 assert(!cnk_zeebx_arm_return_from_api(&c,1u));

 /* Guest event pointers are not host code: an invalid guest callback
  * remains visible and queued, preserving CPU regs for debugging. */
 invoke(&c,11u,0u,0x50000000u,ctx);
 assert(successful_api(&c)==0u);
 memcpy(saved_regs,c.regs,sizeof(saved_regs));
 assert(cnk_zeebx_arm_deliver_timers(&c,64u,&result,&delivered)==-2);
 assert(result.reason==CNK_ARM_UNSUPPORTED &&
        result.pc==0x50000000u && delivered==0u);
 assert(c.timer_count==1u && !memcmp(saved_regs,c.regs,sizeof(saved_regs)));
 invoke(&c,12u,0u,ctx,0u);
 assert(successful_api(&c)==0u && !c.timer_count);

 /* Fixed memory budget: 32 unique timer pairs; the 33rd must trap
  * without evicting valid scheduled guest callbacks. */
 for(uint32_t i=0;i<CNK_ZEEBX_MAX_TIMERS;i++){
  invoke(&c,11u,1000u,fn,ctx+4u*i);
  assert(successful_api(&c)==0u);
 }
 assert(c.timer_count==CNK_ZEEBX_MAX_TIMERS);
 invoke(&c,11u,1000u,fn,ctx+128u);
 assert(!cnk_zeebx_arm_bootstrap(&c,20u,&result,&calls));
 assert(result.reason==CNK_ARM_API && result.api_slot==11u);
 assert(c.timer_count==CNK_ZEEBX_MAX_TIMERS);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx Shell timer API, virtual clock, guest ARM callbacks, "
      "rearm/cancel, safe boundaries, signed deadlines and overflow");
 return 0;
}
