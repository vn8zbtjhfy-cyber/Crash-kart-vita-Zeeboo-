/* Zeebx original ISignal/ISignalCtl/ISignalCBFactory and guest callbacks.
 * No original Crash MOD or copyrighted resources in this test.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void put(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(i*8u));
}
static uint32_t get(cnk_zeebx_arm *c,uint32_t a){
 uint32_t value=0;
 assert(!cnk_zeebx_arm_read_u32(c,a,&value));
 return value;
}
static void call(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                 uint32_t self,uint32_t a1,uint32_t a2,uint32_t a3){
 c->regs[0]=self;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=a3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static void success(cnk_zeebx_arm *c){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED&&handled==1u);
 assert(c->regs[0]==0u);
}
int main(void){
 uint8_t guest[192]={0};
 /* Synthetic guest ARM callback: MOV r1,#42; STR r1,[r0]; BX lr.
  * Actual game callbacks are not reconstructed or pretended. */
 put(guest+64,0xe3a0102au);
 put(guest+68,0xe5801000u);
 put(guest+72,0xe12fff1eu);
 cnk_zeebx_arm c={0};
 assert(!cnk_zeebx_arm_init(&c,guest,sizeof(guest)));
 const uint32_t out_factory=CNK_ARM_OBJECT_BASE+0x300u;
 const uint32_t out_signal=CNK_ARM_OBJECT_BASE+0x304u;
 const uint32_t out_control=CNK_ARM_OBJECT_BASE+0x308u;
 const uint32_t stack_slot=CNK_ARM_STACK_BYTES-16u;
 const uint32_t callback_addr=CNK_ARM_MOD_BASE+64u;
 const uint32_t context=CNK_ARM_HEAP_BASE+0x100u;
 call(&c,0u,2u,CNK_ARM_OBJECT_BASE,0x01041207u,out_factory,0u);
 success(&c);
 uint32_t factory=get(&c,out_factory);
 assert(get(&c,factory)==CNK_ARM_VTABLE_BASE+
        12u*CNK_ARM_VTABLE_STRIDE);

 /* Invalid fifth AAPCS output must not consume object slots. */
 put(c.stack+stack_slot,CNK_ARM_OBJECT_BASE+0xfffu);
 call(&c,12u,3u,factory,callback_addr,context,out_signal);
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(&c,24u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_interface==12u&&out.api_slot==3u);
 assert(c.object_cursor==80u&&c.signal_count==0u);

 put(c.stack+stack_slot,out_control);
 call(&c,12u,3u,factory,callback_addr,context,out_signal);
 success(&c);
 const uint32_t signal=get(&c,out_signal),ctl=get(&c,out_control);
 assert(signal==CNK_ARM_OBJECT_BASE+80u);
 assert(ctl==signal+64u);
 assert(c.object_cursor==208u&&c.signal_count==1u);
 assert(get(&c,signal)==CNK_ARM_VTABLE_BASE+
        10u*CNK_ARM_VTABLE_STRIDE);
 assert(get(&c,ctl)==CNK_ARM_VTABLE_BASE+
        11u*CNK_ARM_VTABLE_STRIDE);

 uint32_t n=123u;
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n));
 assert(out.reason==CNK_ARM_RETURNED&&n==0u);
 call(&c,10u,3u,signal,0u,0u,0u); /* ISignal::Set */
 success(&c);
 assert(c.signal_pending_count==1u);
 uint32_t saved_regs[16];
 memcpy(saved_regs,c.regs,sizeof(saved_regs));
 const uint32_t saved_cpsr=c.cpsr;
 uint64_t before=c.executed;
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n));
 assert(out.reason==CNK_ARM_RETURNED&&n==1u);
 assert(c.executed==before+3u);
 assert(!memcmp(saved_regs,c.regs,sizeof(saved_regs)));
 assert(c.cpsr==saved_cpsr);
 assert(get(&c,context)==42u);
 assert(c.signal_pending_count==0u);
 /* Control::Set uses the same callback, pending until explicit delivery. */
 call(&c,11u,3u,ctl,0u,0u,0u);
 success(&c);
 assert(c.signal_pending_count==1u);
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n)&&n==1u);

 /* Create Zeebo HID factory/device, register the actual signal, then
  * let a physical host pad snapshot enqueue a guest callback. */
 call(&c,0u,2u,CNK_ARM_OBJECT_BASE,0x0106c411u,out_factory,0u);
 success(&c);
 const uint32_t hid=get(&c,out_factory);
 call(&c,8u,3u,hid,1u,out_factory+4u,0u);
 success(&c);
 const uint32_t device=get(&c,out_factory+4u);
 assert(get(&c,device)==CNK_ARM_VTABLE_BASE+
        9u*CNK_ARM_VTABLE_STRIDE);
 /* RegisterForButtonEvent(guest ISignal*) */
 call(&c,9u,8u,device,signal,0u,0u);
 success(&c);
 assert(c.hid_button_signal==signal);
 /* RegisterForPositionChange immediately queues a calibration callback. */
 call(&c,9u,14u,device,signal,0u,0u);
 success(&c);
 assert(c.hid_position_signal==signal&&c.signal_pending_count==1u);
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n)&&n==1u);
 cnk_zeebx_pad pressed={0};
 for(unsigned i=0;i<CNK_ZEEBX_PAD_AXES;i++)pressed.axes[i]=128u;
 pressed.buttons=(1u<<0u)|(1u<<13u);
 pressed.axes[0]=200u;
 assert(!cnk_zeebx_arm_set_pad(&c,&pressed));
 assert(c.hid_queue_count==2u&&c.signal_pending_count==2u);
 before=c.executed;
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n)&&n==2u);
 assert(c.executed==before+6u&&c.signal_pending_count==0u);
 assert(get(&c,context)==42u);
 assert(!cnk_zeebx_arm_set_pad(&c,&pressed));
 assert(c.signal_pending_count==0u);

 /* Full pending callback FIFO must reject new input without modifying
  * physical state or inserting an HID press/release event. */
 for(unsigned i=0;i<CNK_ZEEBX_PENDING_SIGNALS;i++){
  call(&c,10u,3u,signal,0u,0u,0u);
  success(&c);
 }
 assert(c.signal_pending_count==CNK_ZEEBX_PENDING_SIGNALS);
 cnk_zeebx_pad released=pressed;
 released.buttons=0u;
 unsigned old_queue=c.hid_queue_count;
 assert(cnk_zeebx_arm_set_pad(&c,&released)==-2);
 assert(c.pad.buttons==pressed.buttons&&c.hid_queue_count==old_queue);
 /* No false success on a full callback queue. */
 call(&c,10u,3u,signal,0u,0u,0u);
 assert(!cnk_zeebx_arm_bootstrap(&c,24u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_interface==10u&&out.api_slot==3u);
 assert(c.signal_pending_count==CNK_ZEEBX_PENDING_SIGNALS);
 assert(!cnk_zeebx_arm_deliver_signals(&c,30u,&out,&n));
 assert(out.reason==CNK_ARM_BUDGET&&n==10u);
 assert(!cnk_zeebx_arm_deliver_signals(&c,100u,&out,&n));
 assert(out.reason==CNK_ARM_RETURNED&&c.signal_pending_count==0u);

 /* Detach prevents future pad notifications; button events are independent. */
 call(&c,11u,4u,ctl,0u,0u,0u);
 success(&c);
 assert(c.hid_button_signal==0u&&c.hid_position_signal==0u);
 assert(!cnk_zeebx_arm_set_pad(&c,&released));
 assert(c.signal_pending_count==0u);
 call(&c,11u,5u,ctl,0u,0u,0u); /* Enable cannot undo Detach */
 success(&c);
 assert(!cnk_zeebx_arm_set_pad(&c,&pressed));
 assert(c.signal_pending_count==0u);

 /* Corrupt pointer registration cannot install a fake signal callback. */
 call(&c,9u,8u,device,CNK_ARM_OBJECT_BASE+0xeeeu,0u,0u);
 assert(!cnk_zeebx_arm_bootstrap(&c,24u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_interface==9u&&out.api_slot==8u);
 assert(c.hid_button_signal==0u);

 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx guest CreateSignal AAPCS, deferred ARM callbacks, register preservation, HID notifications, finite queue, detach");
 return 0;
}
