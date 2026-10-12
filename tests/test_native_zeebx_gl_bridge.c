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
 unsigned matrix_mode,load_identity,push_matrix,pop_matrix,translate,scale,rotate;
 uint32_t mode;
 float xyz[3],angle;
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
static int matrix_mode(void *p,uint32_t mode){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->matrix_mode++;g->mode=mode;return 0;
}
static int load_identity(void *p){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->load_identity++;return 0;
}
static int push_matrix(void *p){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->push_matrix++;return 0;
}
static int pop_matrix(void *p){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->pop_matrix++;return 0;
}
static int translate(void *p,float x,float y,float z){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->translate++;g->xyz[0]=x;g->xyz[1]=y;g->xyz[2]=z;
 return 0;
}
static int scale(void *p,float x,float y,float z){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->scale++;g->xyz[0]=x;g->xyz[1]=y;g->xyz[2]=z;
 return 0;
}
static int rotate(void *p,float angle,float x,float y,float z){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 g->rotate++;g->angle=angle;
 g->xyz[0]=x;g->xyz[1]=y;g->xyz[2]=z;
 return 0;
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
 const cnk_zeebx_gl_host gpu={
  .make_current=begin,.clear_color=clear_color,.clear=clear,
  .viewport=viewport,.present=present,.matrix_mode=matrix_mode,
  .load_identity=load_identity,.push_matrix=push_matrix,
  .pop_matrix=pop_matrix,.translate=translate,.scale=scale,.rotate=rotate
 };
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
 api(&cpu,24u,51u,0x1700u,0u,0u,0u);
 trap(&cpu,24u,51u); /* matrix commands require current EGL context */
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
 /* Original Zeebx GL_LEGACY matrix slot IDs and signed 16.16 values. */
 api(&cpu,24u,51u,0x1700u,0u,0u,0u); /* GL_MODELVIEW */
 assert(invoke(&cpu)==0u&&g.matrix_mode==1u&&g.mode==0x1700u);
 api(&cpu,24u,51u,0x1701u,0u,0u,0u); /* GL_PROJECTION */
 assert(invoke(&cpu)==0u&&g.matrix_mode==2u);
 api(&cpu,24u,46u,0u,0u,0u,0u);
 assert(invoke(&cpu)==0u&&g.load_identity==1u);
 api(&cpu,24u,61u,0u,0u,0u,0u);
 assert(invoke(&cpu)==0u&&g.push_matrix==1u);
 api(&cpu,24u,77u,(uint32_t)-65536,0u,98304u,0u);
 assert(invoke(&cpu)==0u&&g.translate==1u);
 assert(g.xyz[0]==-1.f&&g.xyz[1]==0.f&&g.xyz[2]==1.5f);
 api(&cpu,24u,65u,131072u,65536u,32768u,0u);
 assert(invoke(&cpu)==0u&&g.scale==1u);
 assert(g.xyz[0]==2.f&&g.xyz[1]==1.f&&g.xyz[2]==0.5f);
 api(&cpu,24u,63u,5898240u,0u,65536u,0u);
 assert(invoke(&cpu)==0u&&g.rotate==1u);
 assert(g.angle==90.f&&g.xyz[0]==0.f&&g.xyz[1]==1.f);
 api(&cpu,24u,60u,0u,0u,0u,0u);
 assert(invoke(&cpu)==0u&&g.pop_matrix==1u);
 api(&cpu,24u,51u,0xdeadbeefu,0u,0u,0u);
 trap(&cpu,24u,51u); /* unexpected matrix enum is never accepted */
 assert(g.matrix_mode==2u);
 api(&cpu,24u,27u,0u,0u,0u,0u); /* glDrawElements UNIMPLEMENTED */
 trap(&cpu,24u,27u);
 api(&cpu,24u,7u,0x80000000u,0u,0u,0u);
 trap(&cpu,24u,7u);
 assert(g.clear==1u);
 g.reject=1;
 api(&cpu,24u,63u,5898240u,0u,65536u,0u);
 trap(&cpu,24u,63u); /* real GPU rejected transform */
 assert(g.rotate==1u);
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
 puts("PASS: Zeebx EGL/GL fixed-point rendering and matrix commands "
      "require real GPU success; invalid enums and missing context trap");
 return 0;
}
