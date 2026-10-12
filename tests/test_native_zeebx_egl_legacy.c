/* Zeebx AEEGL.h legacy IEGL: real guest ABI, ROM-free synthetic memory.
 * Display/config negotiation only. Do NOT pretend GL context is rendered.
 * Source references: Zeebx aee_slots.rs, machine/egl.rs, video/gles.rs.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t at(cnk_zeebx_arm *cpu,uint32_t address){
 uint32_t r=0;
 assert(!cnk_zeebx_arm_read_u32(cpu,address,&r));
 return r;
}
static void wr(uint8_t *p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(8u*i));
}
static void setup(cnk_zeebx_arm *cpu,unsigned iface,unsigned slot,
                  uint32_t a,uint32_t b,uint32_t c,uint32_t d){
 cpu->regs[0]=a;cpu->regs[1]=b;cpu->regs[2]=c;cpu->regs[3]=d;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t invoke(cnk_zeebx_arm *cpu){
 cnk_arm_outcome outcome={0};
 uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,24u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED && handled==1u);
 assert(cpu->regs[15]==CNK_ARM_RETURN_MAGIC);
 return cpu->regs[0];
}
static void expect_trap(cnk_zeebx_arm *cpu,unsigned iface,unsigned slot){
 cnk_arm_outcome outcome={0};
 uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,24u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_API&&handled==0u);
 assert(outcome.api_interface==iface&&outcome.api_slot==slot);
}
int main(void){
 uint8_t code[256]={0};
 cnk_zeebx_arm cpu={0};
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 const uint32_t objects=CNK_ARM_OBJECT_BASE;
 const uint32_t major=objects+0x200u,minor=objects+0x204u;
 const uint32_t configs=objects+0x208u,num=objects+0x20cu;
 const uint32_t attribs=objects+0x300u;
 assert(cpu.egl_error==0x3000u);

 /* The original game's Shell::CreateInstance returns an IEGL guest
  * object with Zeebx's real legacy interface 23 trampoline slots. */
 setup(&cpu,0u,2u,CNK_ARM_OBJECT_BASE,0x01014bc4u,
       objects+0x100u,0u);
 assert(invoke(&cpu)==0u);
 const uint32_t egl=at(&cpu,objects+0x100u);
 assert(at(&cpu,egl)==CNK_ARM_VTABLE_BASE+23u*CNK_ARM_VTABLE_STRIDE);
 assert(at(&cpu,at(&cpu,egl)+4u*5u)==
        CNK_ARM_API_BASE+(23u<<12)+5u*4u);
 /* Legacy EGL calls are plain C function pointers, WITHOUT this in r0. */
 setup(&cpu,23u,3u,0,0,0,0);
 assert(invoke(&cpu)==0x3000u);
 setup(&cpu,23u,4u,0,0,0,0);
 assert(invoke(&cpu)==1u);
 setup(&cpu,23u,5u,1u,major,minor,0);
 assert(invoke(&cpu)==1u);
 assert(at(&cpu,major)==1u && at(&cpu,minor)==0u);
 assert(cpu.egl_initialized==1u);
 setup(&cpu,23u,7u,1u,0x3053u,0u,0u);
 uint32_t vendor=invoke(&cpu);
 assert(vendor>=CNK_ARM_HEAP_BASE&&vendor<CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES);
 assert(!strcmp((char*)cpu.heap+vendor-CNK_ARM_HEAP_BASE,"Zeebx"));
 setup(&cpu,23u,7u,1u,0x3054u,0u,0u);
 uint32_t version=invoke(&cpu);
 assert(!strcmp((char*)cpu.heap+version-CNK_ARM_HEAP_BASE,"1.1"));
 setup(&cpu,23u,7u,1u,0x3055u,0u,0u);
 uint32_t ext=invoke(&cpu);
 assert(cpu.heap[ext-CNK_ARM_HEAP_BASE]==0u); /* never advertise missing GL */
 setup(&cpu,23u,7u,1u,0x3053u,0u,0u);
 assert(invoke(&cpu)==vendor); /* intern, no allocation leak */
 setup(&cpu,23u,9u,1u,configs,1u,num);
 assert(invoke(&cpu)==1u);
 assert(at(&cpu,configs)==1u && at(&cpu,num)==1u);

 /* eglChooseConfig takes the fifth AAPCS argument on guest SP. */
 wr(cpu.objects+0x300u,0x3020u);  /* minimum 16-bit RGB565 */
 wr(cpu.objects+0x304u,16u);
 wr(cpu.objects+0x308u,0x3024u);  /* R >=5 */
 wr(cpu.objects+0x30cu,5u);
 wr(cpu.objects+0x310u,0x3038u);  /* EGL_NONE sentinel */
 wr(cpu.stack+CNK_ARM_STACK_BYTES-16u,num);
 setup(&cpu,23u,10u,1u,attribs,configs,1u);
 assert(invoke(&cpu)==1u);
 assert(at(&cpu,configs)==1u&&at(&cpu,num)==1u);

 /* Unrealistic requirement returns no matches, not made-up config. */
 wr(cpu.objects+0x304u,32u);
 setup(&cpu,23u,10u,1u,attribs,configs,1u);
 assert(invoke(&cpu)==1u);
 assert(at(&cpu,num)==0u);
 wr(cpu.objects+0x304u,16u);

 setup(&cpu,23u,11u,1u,1u,0x3023u,major);
 assert(invoke(&cpu)==1u&&at(&cpu,major)==6u); /* green bits */
 setup(&cpu,23u,11u,1u,1u,0x302bu,major);
 assert(invoke(&cpu)==1u&&at(&cpu,major)==307200u);
 setup(&cpu,23u,11u,1u,1u,0x9999u,major);
 assert(invoke(&cpu)==0u);
 setup(&cpu,23u,3u,0,0,0,0);
 assert(invoke(&cpu)==0x3004u && cpu.egl_error==0x3000u);
 setup(&cpu,23u,3u,0,0,0,0);
 assert(invoke(&cpu)==0x3000u);

 /* Bad output pointers cannot create partial guest writes. */
 setup(&cpu,23u,5u,1u,objects+0xffcu,objects+0xffeu,0);
 expect_trap(&cpu,23u,5u);
 setup(&cpu,23u,10u,1u,objects+0xffcu,configs,1u);
 expect_trap(&cpu,23u,10u); /* malformed attrs at edge, no terminator */
 setup(&cpu,23u,26u,1u,0u,0u,0u);
 expect_trap(&cpu,23u,26u); /* cannot swap a nonexistent GPU surface */
 setup(&cpu,23u,17u,1u,1u,0u,0u);
 expect_trap(&cpu,23u,17u); /* no fake GL context or frame */

 setup(&cpu,23u,22u,0,0,0,0);
 assert(invoke(&cpu)==1u);
 setup(&cpu,23u,6u,1u,0,0,0);
 assert(invoke(&cpu)==1u);
 setup(&cpu,23u,22u,0,0,0,0);
 assert(invoke(&cpu)==0u);
 setup(&cpu,23u,7u,1u,0x3053u,0u,0u);
 assert(invoke(&cpu)==0u);
 assert(cpu.egl_queries>=16u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: Zeebx guest legacy IEGL RGB565 config, AAPCS fifth argument, "
      "query strings, real error and unsupported GPU traps");
 return 0;
}
