/* Crash's original Zeebx gl.rs uses TexEnvxv(pointer) for GL_REPLACE.
 * Test actual BREW IGL slot dispatch with synthetic guest memory,
 * not private game resources or a fake completed race. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {unsigned calls;uint32_t last;int fail;} recorder;
static int begin(void *u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int present(void *u){(void)u;return 0;}
static int clear(void *u,uint32_t mask){(void)u;(void)mask;return 0;}
static int color(void *u,float r,float g,float b,float a){
 (void)u;(void)r;(void)g;(void)b;(void)a;return 0;
}
static int viewport(void *u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int texenv(void *u,uint32_t mode){
 recorder *r=(recorder*)u;
 if(r->fail)return -1;
 r->calls++;r->last=mode;return 0;
}
static void api(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                uint32_t a0,uint32_t a1,uint32_t a2){
 c->regs[0]=a0;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=0u;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t ok(cnk_zeebx_arm *c){
 cnk_arm_outcome o={0};uint32_t count=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&count));
 assert(o.reason==CNK_ARM_RETURNED&&count==1u);
 return c->regs[0];
}
static void trapped(cnk_zeebx_arm *c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t count=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&count));
 assert(o.reason==CNK_ARM_API&&o.api_interface==24u&&o.api_slot==slot);
 assert(count==0u);
}
static void put32(uint8_t *p,uint32_t n){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(n>>(i*8u));
}
int main(void){
 uint8_t mod[128]={0};
 cnk_zeebx_arm cpu={0};
 recorder g={0};
 const cnk_zeebx_gl_host host={
  .make_current=begin,.present=present,.clear=clear,
  .clear_color=color,.viewport=viewport,.texture_env_mode=texenv
 };
 assert(!cnk_zeebx_arm_init(&cpu,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&host,&g));
 /* GL requires actual current EGL context: no fake pre-init success. */
 api(&cpu,24u,72u,0x2300u,0x2200u,0x1e01u);
 trapped(&cpu,72u);
 assert(g.calls==0u);
 api(&cpu,23u,5u,1u,0u,0u);assert(ok(&cpu)==1u);
 api(&cpu,23u,12u,1u,1u,0u);
 const uint32_t surface=ok(&cpu);
 api(&cpu,23u,17u,1u,1u,0u);
 const uint32_t context=ok(&cpu);
 api(&cpu,23u,19u,1u,surface,surface);
 cpu.regs[3]=context;
 assert(ok(&cpu)==1u);
 /* Scalar x variant sends raw GL enums, NEVER divide by 65536. */
 api(&cpu,24u,72u,0x2300u,0x2200u,0x1e01u);
 assert(ok(&cpu)==0u&&g.calls==1u&&g.last==0x1e01u);
 api(&cpu,24u,72u,0x2300u,0x2200u,0x2100u);
 assert(ok(&cpu)==0u&&g.calls==2u&&g.last==0x2100u);
 /* Actual pointer variant Crash uses. Read the guest's value, not
  * the pointer address as a color or fixed-point scale. */
 const uint32_t ptr=CNK_ARM_HEAP_BASE+64u;
 put32(cpu.heap+64u,0x1e01u);
 api(&cpu,24u,73u,0x2300u,0x2200u,ptr);
 assert(ok(&cpu)==0u&&g.calls==3u&&g.last==0x1e01u);
 put32(cpu.heap+64u,0x0104u);
 api(&cpu,24u,73u,0x2300u,0x2200u,ptr);
 assert(ok(&cpu)==0u&&g.calls==4u&&g.last==0x0104u);
 /* All guest bounds checks run BEFORE the host changes GPU state. */
 api(&cpu,24u,73u,0x2300u,0x2200u,ptr+1u);
 trapped(&cpu,73u);
 api(&cpu,24u,73u,0x2300u,0x2200u,0u);
 trapped(&cpu,73u);
 api(&cpu,24u,73u,0x2300u,0x2200u,
     CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES);
 trapped(&cpu,73u);
 api(&cpu,24u,72u,0xdeadu,0x2200u,0x1e01u);
 trapped(&cpu,72u);
 api(&cpu,24u,73u,0x2300u,0xdeadu,ptr);
 trapped(&cpu,73u);
 api(&cpu,24u,72u,0x2300u,0x2200u,0xdeadbeefu);
 trapped(&cpu,72u);
 put32(cpu.heap+64u,0xdeadbeefu);
 api(&cpu,24u,73u,0x2300u,0x2200u,ptr);
 trapped(&cpu,73u);
 assert(g.calls==4u);
 /* Neither absent GPU callback nor rejected hardware result may
  * produce fake success or claim a reconstructed graphic. */
 cpu.gl_host.texture_env_mode=NULL;
 api(&cpu,24u,72u,0x2300u,0x2200u,0x1e01u);
 trapped(&cpu,72u);
 cpu.gl_host.texture_env_mode=texenv;
 g.fail=1;
 api(&cpu,24u,72u,0x2300u,0x2200u,0x1e01u);
 trapped(&cpu,72u);
 put32(cpu.heap+64u,0x1e01u);
 api(&cpu,24u,73u,0x2300u,0x2200u,ptr);
 trapped(&cpu,73u);
 assert(g.calls==4u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: original Crash GL_REPLACE via Zeebx TexEnvx/TexEnvxv,"
      " guest-pointer bounds, enum validation and GPU failures");
 return 0;
}
