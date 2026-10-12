/* Zeebx AEEGL.h legacy C ABI forwarded to a REAL host rendering sink.
 * Synthetic guest state and a recorded GPU only; no proprietary ROM files.
 * Host callback failures must trap, not report an imaginary drawn frame.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 unsigned begin,clear_color,clear,viewport,present;
 uint32_t clear_mask;
 uint32_t w,h;
 int32_t x,y;
 float color[4];
 int reject;
} recorded_gpu;
static int begin(void *p,uint32_t w,uint32_t h){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->begin++;g->w=w;g->h=h;return 0;
}
static int clear_color(void *p,float r,float g,float b,float a){
 recorded_gpu *gpu=(recorded_gpu*)p;
 if(gpu->reject)return -1;
 gpu->clear_color++;
 gpu->color[0]=r;gpu->color[1]=g;gpu->color[2]=b;gpu->color[3]=a;
 return 0;
}
static int clear(void *p,uint32_t mask){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->clear++;g->clear_mask=mask;return 0;
}
static int viewport(void *p,int32_t x,int32_t y,uint32_t w,uint32_t h){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->viewport++;g->x=x;g->y=y;g->w=w;g->h=h;return 0;
}
static int present(void *p){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->present++;return 0;
}
static void api(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3){
 c->regs[0]=a0;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=a3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t invoke(cnk_zeebx_arm *c){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&n));
 assert(o.reason==CNK_ARM_RETURNED&&n==1u);
 return c->regs[0];
}
static void trap(cnk_zeebx_arm *c,unsigned iface,unsigned slot){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&n));
 assert(o.reason==CNK_ARM_API&&n==0u);
 assert(o.api_interface==iface&&o.api_slot==slot);
}
int main(void){
 uint8_t guest[256]={0};
 cnk_zeebx_arm cpu={0};
 recorded_gpu g={0};
 const cnk_zeebx_gl_host gpu={begin,clear_color,clear,viewport,present};
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 api(&cpu,23u,5u,1u,0u,0u,0u); /* original EGL initialize */
 assert(invoke(&cpu)==1u);
 api(&cpu,23u,12u,1u,1u,0u,0u);
 trap(&cpu,23u,12u); /* NO device means NO surface */
 assert(cpu.egl_surface==0u);
 cnk_zeebx_gl_host incomplete=gpu;incomplete.present=NULL;
 assert(cnk_zeebx_arm_set_gl_host(&cpu,&incomplete,&g)==-3);
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&gpu,&g));
 api(&cpu,23u,12u,1u,1u,0u,0u); /* CreateWindowSurface */
 uint32_t surf=invoke(&cpu);
 assert(surf==2u);
 api(&cpu,23u,17u,1u,1u,0u,0u); /* CreateContext */
 uint32_t ctx=invoke(&cpu);
 assert(ctx==3u);
 api(&cpu,23u,26u,1u,surf,0u,0u);
 assert(invoke(&cpu)==0u); /* no frame until MakeCurrent */
 assert(g.present==0u);
 api(&cpu,24u,7u,0x4000u,0u,0u,0u);
 trap(&cpu,24u,7u);
 api(&cpu,23u,19u,1u,surf,surf,ctx);
 assert(invoke(&cpu)==1u&&g.begin==1u&&g.w==640u&&g.h==480u);
 api(&cpu,23u,20u,0u,0u,0u,0u);
 assert(invoke(&cpu)==ctx);
 api(&cpu,23u,21u,0u,0u,0u,0u);
 assert(invoke(&cpu)==surf);
 /* Legacy GL fixed point 16.16; clamp negative and >1 just like GLES. */
 api(&cpu,24u,8u,65536u,32768u,0xffffffffu,131072u);
 assert(invoke(&cpu)==0u&&g.clear_color==1u);
 assert(g.color[0]==1.f && g.color[1]==0.5f &&
        g.color[2]==0.f && g.color[3]==1.f);
 api(&cpu,24u,79u,10u,20u,320u,240u);
 assert(invoke(&cpu)==0u && g.viewport==1u);
 assert(g.x==10 && g.y==20 && g.w==320 && g.h==240);
 api(&cpu,24u,7u,0x4000u|0x100u,0u,0u,0u);
 assert(invoke(&cpu)==0u&&g.clear==1u);
 assert(g.clear_mask==(0x4000u|0x100u));
 api(&cpu,24u,37u,0u,0u,0u,0u); /* glGetError */
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,27u,0u,0u,0u,0u); /* glDrawElements UNIMPLEMENTED */
 trap(&cpu,24u,27u);
 api(&cpu,24u,7u,0x80000000u,0u,0u,0u);
 trap(&cpu,24u,7u);
 assert(g.clear==1u);
 g.reject=1;
 api(&cpu,24u,7u,0x4000u,0u,0u,0u);
 trap(&cpu,24u,7u);
 assert(g.clear==1u&&cpu.gl_clear_calls==1u);
 api(&cpu,23u,26u,1u,surf,0u,0u);
 trap(&cpu,23u,26u); /* failed real GPU swap is NOT successful */
 assert(g.present==0u&&cpu.gl_presents==0u);
 g.reject=0;
 api(&cpu,23u,26u,1u,surf,0u,0u);
 assert(invoke(&cpu)==1u && g.present==1u && cpu.gl_presents==1u);
 assert(cpu.gl_viewport_calls==1u);
 assert(cnk_zeebx_arm_set_gl_host(&cpu,NULL,NULL)==-2);
 api(&cpu,23u,15u,1u,surf,0u,0u);
 assert(invoke(&cpu)==0u); /* bound surface cannot be destroyed */
 api(&cpu,23u,19u,1u,0u,0u,0u);
 assert(invoke(&cpu)==1u); /* unbind */
 api(&cpu,23u,18u,1u,ctx,0u,0u);
 assert(invoke(&cpu)==1u);
 api(&cpu,23u,15u,1u,surf,0u,0u);
 assert(invoke(&cpu)==1u);
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,NULL,NULL));
 api(&cpu,23u,17u,1u,1u,0u,0u);
 trap(&cpu,23u,17u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: Zeebx EGL window/context and IGL fixed-point commands are sent "
      "to a real host sink only; no GPU means explicit traps");
 return 0;
}
