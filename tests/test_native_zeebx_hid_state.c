/* Zeebx original BREW IHIDDevice ABI: 18 button UIDs and 25-word
 * position ranges. Synthetic guest objects ONLY; no proprietary ROM.
 * The virtual pad is populated by the PS Vita adapter separately.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void api(cnk_zeebx_arm *cpu,uint32_t obj,unsigned iface,
                unsigned slot,uint32_t a1,uint32_t a2){
 cpu->regs[0]=obj;
 cpu->regs[1]=a1;
 cpu->regs[2]=a2;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static void expect_success(cnk_zeebx_arm *cpu){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,12u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED && handled==1u);
 assert(cpu->regs[0]==0u);
}
static uint32_t at(cnk_zeebx_arm *cpu,uint32_t addr){
 uint32_t v=0;
 assert(!cnk_zeebx_arm_read_u32(cpu,addr,&v));
 return v;
}

int main(void){
 uint8_t synthetic_mod[128]={0};
 cnk_zeebx_arm c={0};
 assert(!cnk_zeebx_arm_init(&c,synthetic_mod,sizeof(synthetic_mod)));
 const uint32_t output=CNK_ARM_OBJECT_BASE+0x300u;
 const uint32_t info=CNK_ARM_OBJECT_BASE+0x380u;
 for(unsigned i=0;i<CNK_ZEEBX_PAD_AXES;i++)
  assert(c.pad.axes[i]==128u);
 assert(c.pad.buttons==0u);

 /* Real guest-side class creation, then IHID CreateDevice(handle=1). */
 api(&c,CNK_ARM_OBJECT_BASE,0u,2u,0x0106c411u,output);
 expect_success(&c);
 const uint32_t hid=at(&c,output);
 assert(at(&c,hid)==CNK_ARM_VTABLE_BASE+8u*CNK_ARM_VTABLE_STRIDE);
 api(&c,hid,8u,3u,1u,output+4u);
 expect_success(&c);
 const uint32_t dev=at(&c,output+4u);
 assert(at(&c,dev)==CNK_ARM_VTABLE_BASE+9u*CNK_ARM_VTABLE_STRIDE);

 api(&c,dev,9u,7u,info,0u);
 expect_success(&c);
 assert(at(&c,info)==18u);
 api(&c,dev,9u,10u,info,0u);
 expect_success(&c);
 assert(at(&c,info)==0u);
 for(unsigned word=1u;word<25u;word++)
  assert(at(&c,info+4u*word)==128u);

 api(&c,dev,9u,13u,info,0u); /* GetAxesInfo: UIDs not positions */
 expect_success(&c);
 assert(at(&c,info)==0u);
 assert(at(&c,info+4u)==0x0106c4d0u);
 assert(at(&c,info+8u)==0x0106c4d1u);
 assert(at(&c,info+12u)==0x0106c4ceu);
 assert(at(&c,info+24u)==0x0106c4cfu);
 for(unsigned word=7u;word<25u;word++)
  assert(at(&c,info+4u*word)==0u);
 api(&c,dev,9u,11u,info,0u); /* Min */
 expect_success(&c);
 for(unsigned word=0;word<25;word++)assert(at(&c,info+word*4u)==0u);
 api(&c,dev,9u,12u,info,0u); /* Max */
 expect_success(&c);
 assert(at(&c,info)==0u);
 for(unsigned word=1;word<25;word++)
  assert(at(&c,info+word*4u)==255u);

 const cnk_zeebx_pad active={
  (1u<<0)|(1u<<13)|(1u<<11),{0u,255u,37u,219u}
 };
 assert(!cnk_zeebx_arm_set_pad(&c,&active));
 api(&c,dev,9u,10u,info,0u);
 expect_success(&c);
 assert(at(&c,info+4u)==0u);
 assert(at(&c,info+8u)==255u);
 assert(at(&c,info+12u)==37u);
 assert(at(&c,info+24u)==219u);
 assert(at(&c,info+16u)==128u); /* unused Ry remains centered */

 /* GetButtonInfo accepts both Zeebo index and UID, preserving
  * original enumerated ordering, booleans and bounds. */
 api(&c,dev,9u,6u,0u,info);
 expect_success(&c);
 assert(at(&c,info)==0u&&at(&c,info+4u)==1u);
 assert(at(&c,info+8u)==0x0106c40bu);
 assert(at(&c,info+12u)==0u&&at(&c,info+16u)==1u);
 api(&c,dev,9u,6u,0x0106c3ffu,info); /* D-pad left UID */
 expect_success(&c);
 assert(at(&c,info)==13u&&at(&c,info+4u)==1u);
 api(&c,dev,9u,6u,3u,info); /* unpressed face button */
 expect_success(&c);
 assert(at(&c,info)==3u&&at(&c,info+4u)==0u);
 assert(at(&c,info+8u)==0x0106c40du);

 /* Unknown buttons, unknown future event methods and invalid memory
  * must STOP at exact BREW API, without changing guest memory. */
 cnk_arm_outcome out={0};uint32_t handled=0;
 api(&c,dev,9u,6u,99u,info);
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_slot==6u);
 assert(at(&c,info)==3u);
 const uint32_t unsafe=CNK_ARM_OBJECT_BASE+0xff0u;
 api(&c,dev,9u,10u,unsafe,0u);
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_slot==10u);
 api(&c,dev,9u,13u,info+1u,0u);
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_slot==13u);
 /* The first real host snapshot pressed buttons 0, 11 and 13.
  * Zeebx delivers press/release events in ascending HID index order. */
 assert(c.hid_queue_count==3u);
 const uint32_t stamp=CNK_ARM_OBJECT_BASE+0x600u;
 const uint32_t dropped=CNK_ARM_OBJECT_BASE+0x604u;
 api(&c,dev,9u,9u,unsafe,stamp);
 c.regs[3]=dropped;
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_slot==9u);
 assert(c.hid_queue_count==3u); /* invalid output must NOT pop */
 const unsigned first_three[]={0u,11u,13u};
 for(unsigned i=0;i<3u;i++){
  api(&c,dev,9u,9u,info,stamp);
  c.regs[3]=dropped;
  expect_success(&c);
  assert(at(&c,info)==first_three[i]);
  assert(at(&c,info+4u)==1u);
  assert(at(&c,stamp)==0u && at(&c,dropped)==0u);
 }
 assert(c.hid_queue_count==0u);
 api(&c,dev,9u,9u,info,stamp);
 c.regs[3]=dropped;
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED && handled==1u);
 assert(c.regs[0]==1u); /* EFAILED: no events, not a fake press */
 assert(at(&c,info)==0u && at(&c,info+4u)==0u);
 /* Repeating same physical snapshot must not duplicate events. */
 assert(!cnk_zeebx_arm_set_pad(&c,&active));
 assert(c.hid_queue_count==0u);
 assert(!cnk_zeebx_arm_set_virtual_time(&c,1234u));
 cnk_zeebx_pad released=active;
 released.buttons=0u;
 assert(!cnk_zeebx_arm_set_pad(&c,&released));
 assert(c.hid_queue_count==3u);
 for(unsigned i=0;i<3u;i++){
  api(&c,dev,9u,9u,info,stamp);
  c.regs[3]=dropped;
  expect_success(&c);
  assert(at(&c,info)==first_three[i] && at(&c,info+4u)==0u);
  assert(at(&c,stamp)==1234u);
 }

 /* Overflow is finite and visible, keeps freshest events. */
 cnk_zeebx_pad moving=released;
 for(unsigned i=0;i<70u;i++){
  moving.buttons^=1u;
  assert(!cnk_zeebx_arm_set_pad(&c,&moving));
 }
 assert(c.hid_queue_count==CNK_ZEEBX_HID_QUEUE);
 assert(c.hid_events_dropped==1u);
 api(&c,dev,9u,9u,info,stamp);
 c.regs[3]=dropped;
 expect_success(&c);
 assert(at(&c,info)==0u);
 assert(at(&c,dropped)==1u);
 assert(c.hid_queue_count==CNK_ZEEBX_HID_QUEUE-1u);
 api(&c,dev,9u,9u,info,stamp);
 c.regs[3]=dropped;
 expect_success(&c);
 assert(at(&c,dropped)==0u);
 /* An actually unimplemented BREW method remains an explicit trap. */
 api(&c,dev,9u,8u,info,stamp);
 assert(!cnk_zeebx_arm_bootstrap(&c,12u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_slot==8u);

 cnk_zeebx_pad invalid=active;
 invalid.buttons=1u<<18;
 assert(cnk_zeebx_arm_set_pad(&c,&invalid)<0);
 assert(c.pad.buttons==moving.buttons);
 assert(cnk_zeebx_arm_set_pad(&c,NULL)<0);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx guest HID: physical snapshot, 18 button UIDs, four axes,"
      " 25-word ranges, invalid-guest-memory traps");
 return 0;
}
