/* ROM-free original Zeebx IGL Common-Lite fixed-function pipeline ABI.
 * A recorded host proves forwarded arguments and verified errors.
 * No test claims an actual PS Vita framebuffer or a playable race.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 unsigned cap,blend,depth,mask,cull,face,color,color_mask;
 uint32_t a,b;
 float rgba[4];
 int flags[4],enabled,reject;
} gpu_log;
static int current(void *p,uint32_t w,uint32_t h){
 gpu_log *g=(gpu_log*)p;
 return g->reject||w!=640u||h!=480u?-1:0;
}
static int present(void *p){return ((gpu_log*)p)->reject?-1:0;}
static int clear(void *p,uint32_t mask){
 (void)mask;return ((gpu_log*)p)->reject?-1:0;
}
static int clear_color(void *p,float a,float b,float c,float d){
 (void)a;(void)b;(void)c;(void)d;
 return ((gpu_log*)p)->reject?-1:0;
}
static int viewport(void *p,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)x;(void)y;(void)w;(void)h;
 return ((gpu_log*)p)->reject?-1:0;
}
static int capability(void *p,uint32_t cap,int on){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->cap++;g->a=cap;g->enabled=on;return 0;
}
static int blend(void *p,uint32_t src,uint32_t dst){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->blend++;g->a=src;g->b=dst;return 0;
}
static int depth(void *p,uint32_t func){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->depth++;g->a=func;return 0;
}
static int mask(void *p,int on){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->mask++;g->enabled=on;return 0;
}
static int cull(void *p,uint32_t face){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->cull++;g->a=face;return 0;
}
static int front(void *p,uint32_t face){
 gpu_log *g=(gpu_log*)p;
 if(g->reject)return -1;
 g->face++;g->a=face;return 0;
}
static int color(void *p,float r,float g,float b,float a){
 gpu_log *log=(gpu_log*)p;
 if(log->reject)return -1;
 log->color++;log->rgba[0]=r;log->rgba[1]=g;
 log->rgba[2]=b;log->rgba[3]=a;
 return 0;
}
static int color_mask(void *p,int r,int g,int b,int a){
 gpu_log *log=(gpu_log*)p;
 if(log->reject)return -1;
 log->color_mask++;
 log->flags[0]=r;log->flags[1]=g;log->flags[2]=b;log->flags[3]=a;
 return 0;
}
static void api(cnk_zeebx_arm *cpu,unsigned iface,unsigned slot,
                uint32_t a,uint32_t b,uint32_t c,uint32_t d){
 cpu->regs[0]=a;cpu->regs[1]=b;cpu->regs[2]=c;cpu->regs[3]=d;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t call(cnk_zeebx_arm *cpu){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,24u,&out,&handled));
 assert(out.reason==CNK_ARM_RETURNED&&handled==1u);
 return cpu->regs[0];
}
static void trap(cnk_zeebx_arm *cpu,unsigned slot){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(cpu,24u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&handled==0u);
 assert(out.api_interface==24u&&out.api_slot==slot);
}
int main(void){
 uint8_t dummy[256]={0};
 cnk_zeebx_arm cpu={0};
 gpu_log g={0};
 cnk_zeebx_gl_host gpu={
  .make_current=current,.present=present,.clear=clear,
  .clear_color=clear_color,.viewport=viewport,
  .set_capability=capability,.blend_func=blend,
  .depth_func=depth,.depth_mask=mask,
  .cull_face=cull,.front_face=front,
  .color4=color,.color_mask=color_mask
 };
 assert(!cnk_zeebx_arm_init(&cpu,dummy,sizeof(dummy)));
 /* There is NO native GPU until real Vita host has been registered. */
 api(&cpu,23u,5u,1u,0u,0u,0u);
 assert(call(&cpu)==1u);
 api(&cpu,24u,28u,0x0b71u,0u,0u,0u);
 trap(&cpu,28u);
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&gpu,&g));
 api(&cpu,23u,12u,1u,1u,0u,0u);
 uint32_t surf=call(&cpu);
 api(&cpu,23u,17u,1u,1u,0u,0u);
 uint32_t ctx=call(&cpu);
 api(&cpu,23u,19u,1u,surf,surf,ctx);
 assert(call(&cpu)==1u);
 /* Enable/Disable GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE only.
  * TEXTURE_2D MUST stay unsupported until Zeebx texture binding works. */
 const uint32_t caps[]={0x0b71u,0x0be2u,0x0b44u};
 for(unsigned i=0;i<3u;i++){
  api(&cpu,24u,28u,caps[i],0u,0u,0u);
  assert(call(&cpu)==0u&&g.a==caps[i]&&g.enabled==1);
  api(&cpu,24u,24u,caps[i],0u,0u,0u);
  assert(call(&cpu)==0u&&g.a==caps[i]&&g.enabled==0);
 }
 assert(g.cap==6u);
 api(&cpu,24u,28u,0x0de1u,0u,0u,0u); /* texture not functional */
 trap(&cpu,28u);
 api(&cpu,24u,24u,0xdeadbeefu,0u,0u,0u);
 trap(&cpu,24u);
 assert(g.cap==6u);
 /* Alpha blend forward with GLES1 supported factors only. */
 api(&cpu,24u,6u,0x0302u,0x0303u,0u,0u);
 assert(call(&cpu)==0u&&g.blend==1u);
 assert(g.a==0x0302u&&g.b==0x0303u);
 api(&cpu,24u,6u,1u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.blend==2u&&g.a==1u&&g.b==0u);
 api(&cpu,24u,6u,0x0308u,0x0303u,0u,0u);
 trap(&cpu,6u);
 assert(g.blend==2u);
 /* All eight core depth comparisons are legal; unknown is rejected. */
 for(uint32_t mode=0x0200u;mode<=0x0207u;mode++){
  api(&cpu,24u,21u,mode,0u,0u,0u);
  assert(call(&cpu)==0u&&g.a==mode);
 }
 assert(g.depth==8u);
 api(&cpu,24u,21u,0x0302u,0u,0u,0u);
 trap(&cpu,21u);
 api(&cpu,24u,22u,1u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.mask==1u&&g.enabled==1);
 api(&cpu,24u,22u,0u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.mask==2u&&g.enabled==0);
 api(&cpu,24u,22u,2u,0u,0u,0u);
 trap(&cpu,22u);
 assert(g.mask==2u);
 /* Both culling side and winding must reach host unchanged. */
 for(uint32_t side=0x0404u;side<=0x0405u;side++){
  api(&cpu,24u,19u,side,0u,0u,0u);
  assert(call(&cpu)==0u&&g.a==side);
 }
 api(&cpu,24u,19u,0x0408u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.cull==3u);
 api(&cpu,24u,19u,0x0406u,0u,0u,0u);
 trap(&cpu,19u);
 api(&cpu,24u,34u,0x0900u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.face==1u&&g.a==0x0900u);
 api(&cpu,24u,34u,0x0901u,0u,0u,0u);
 assert(call(&cpu)==0u&&g.face==2u&&g.a==0x0901u);
 api(&cpu,24u,34u,0x0902u,0u,0u,0u);
 trap(&cpu,34u);
 /* Original color4x is SIGNED fixed-point 16.16, clamped to 0-1. */
 api(&cpu,24u,12u,65536u,32768u,(uint32_t)-65536,131072u);
 assert(call(&cpu)==0u&&g.color==1u);
 assert(g.rgba[0]==1.f&&g.rgba[1]==0.5f&&
        g.rgba[2]==0.f&&g.rgba[3]==1.f);
 api(&cpu,24u,13u,1u,0u,1u,0u);
 assert(call(&cpu)==0u&&g.color_mask==1u);
 assert(g.flags[0]==1&&g.flags[1]==0&&g.flags[2]==1&&g.flags[3]==0);
 api(&cpu,24u,13u,1u,0u,2u,0u);
 trap(&cpu,13u);
 assert(g.color_mask==1u);
 /* Missing GPU implementations must stop explicitly without mutation. */
 cnk_zeebx_gl_host saved=cpu.gl_host;
 cpu.gl_host.set_capability=NULL;
 api(&cpu,24u,28u,0x0b71u,0u,0u,0u);
 trap(&cpu,28u);
 cpu.gl_host=saved;
 /* Real host failure is not treated as an accepted GPU state change. */
 g.reject=1;
 api(&cpu,24u,28u,0x0b71u,0u,0u,0u);
 trap(&cpu,28u);
 api(&cpu,24u,6u,0x0302u,0x0303u,0u,0u);
 trap(&cpu,6u);
 api(&cpu,24u,12u,65536u,65536u,65536u,65536u);
 trap(&cpu,12u);
 api(&cpu,24u,22u,0u,0u,0u,0u);
 trap(&cpu,22u);
 assert(g.cap==6u&&g.blend==2u&&g.color==1u&&g.mask==2u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: original Zeebo fixed-pipeline IGL states forwarded to real "
      "GPU callbacks; invalid enum and GPU failure never fake success");
 return 0;
}
