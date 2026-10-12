/* Original Zeebx Common-Lite GLES1 (AEEGL.h) projection/matrix ABI.
 * ROM-free guest memory and recording GPU; no imagined rendered frame.
 * glFrustumx and glOrthox use fifth/sixth ARM AAPCS stack arguments.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 unsigned make_current,load_matrix,mult_matrix,frustum,ortho;
 float matrix[16],projection[6];
 int reject;
} recording_gpu;

static int begin(void *ctx,uint32_t w,uint32_t h){
 recording_gpu *g=(recording_gpu*)ctx;
 if(g->reject || w!=640u || h!=480u)return -1;
 g->make_current++;return 0;
}
static int ok_color(void *u,float r,float g,float b,float a){
 (void)u;(void)r;(void)g;(void)b;(void)a;return 0;
}
static int ok_clear(void *u,uint32_t mask){
 (void)u;(void)mask;return 0;
}
static int ok_viewport(void *u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int ok_present(void *u){(void)u;return 0;}
static int load_matrix(void *u,const float *m){
 recording_gpu *g=(recording_gpu*)u;
 if(g->reject)return -1;
 g->load_matrix++;memcpy(g->matrix,m,sizeof(g->matrix));return 0;
}
static int mult_matrix(void *u,const float *m){
 recording_gpu *g=(recording_gpu*)u;
 if(g->reject)return -1;
 g->mult_matrix++;memcpy(g->matrix,m,sizeof(g->matrix));return 0;
}
static int frustum(void *u,float l,float r,float b,float t,float n,float f){
 recording_gpu *g=(recording_gpu*)u;
 if(g->reject)return -1;
 g->frustum++;
 g->projection[0]=l;g->projection[1]=r;g->projection[2]=b;
 g->projection[3]=t;g->projection[4]=n;g->projection[5]=f;
 return 0;
}
static int ortho(void *u,float l,float r,float b,float t,float n,float f){
 recording_gpu *g=(recording_gpu*)u;
 if(g->reject)return -1;
 g->ortho++;
 g->projection[0]=l;g->projection[1]=r;g->projection[2]=b;
 g->projection[3]=t;g->projection[4]=n;g->projection[5]=f;
 return 0;
}
static void wr(uint8_t *ptr,uint32_t bits){
 for(unsigned j=0;j<4u;j++)ptr[j]=(uint8_t)(bits>>(j*8u));
}
static void api(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3){
 c->regs[0]=a0;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=a3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12u)+slot*4u;
}
static uint32_t success(cnk_zeebx_arm *c){
 cnk_arm_outcome outcome={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,16u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_RETURNED&&handled==1u);
 return c->regs[0];
}
static void trapped(cnk_zeebx_arm *c,unsigned slot){
 cnk_arm_outcome outcome={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,16u,&outcome,&handled));
 assert(outcome.reason==CNK_ARM_API&&handled==0u);
 assert(outcome.api_interface==24u&&outcome.api_slot==slot);
}
int main(void){
 uint8_t module[256]={0};
 cnk_zeebx_arm cpu={0};
 recording_gpu gpu={0};
 const cnk_zeebx_gl_host bridge={
  .make_current=begin,.clear_color=ok_color,.clear=ok_clear,
  .viewport=ok_viewport,.present=ok_present,
  .load_matrix=load_matrix,.mult_matrix=mult_matrix,
  .frustum=frustum,.ortho=ortho
 };
 assert(!cnk_zeebx_arm_init(&cpu,module,sizeof(module)));
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&bridge,&gpu));
 const uint32_t matrix_ptr=CNK_ARM_HEAP_BASE+256u;
 api(&cpu,24u,47u,matrix_ptr,0u,0u,0u);
 trapped(&cpu,47u); /* must have a current real EGL context first */
 assert(gpu.load_matrix==0u);

 api(&cpu,23u,5u,1u,0u,0u,0u);
 assert(success(&cpu)==1u);
 api(&cpu,23u,12u,1u,1u,0u,0u);
 const uint32_t surf=success(&cpu);
 assert(surf!=0u);
 api(&cpu,23u,17u,1u,1u,0u,0u);
 const uint32_t ctx=success(&cpu);
 assert(ctx!=0u);
 api(&cpu,23u,19u,1u,surf,surf,ctx);
 assert(success(&cpu)==1u&&gpu.make_current==1u);

 /* Source data is always sixteen signed 16.16 *guest* little-endian
  * numbers, column-major. Never forward guest pointers to the GPU. */
 int32_t matrix[16]={
  65536,0,0,0,0,-98304,0,0,0,0,131072,0,32768,0,0,65536
 };
 for(unsigned j=0;j<16u;j++)
  wr(cpu.heap+256u+j*4u,(uint32_t)matrix[j]);
 api(&cpu,24u,47u,matrix_ptr,0u,0u,0u);
 assert(success(&cpu)==0u&&gpu.load_matrix==1u);
 assert(gpu.matrix[0]==1.f && gpu.matrix[5]==-1.5f &&
        gpu.matrix[10]==2.f && gpu.matrix[12]==0.5f);
 api(&cpu,24u,52u,matrix_ptr,0u,0u,0u);
 assert(success(&cpu)==0u&&gpu.mult_matrix==1u);
 assert(gpu.matrix[15]==1.f);
 /* Guest memory overrun and alignment fail *before* host access. */
 api(&cpu,24u,47u,CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-60u,0u,0u,0u);
 trapped(&cpu,47u);
 api(&cpu,24u,52u,matrix_ptr+1u,0u,0u,0u);
 trapped(&cpu,52u);
 api(&cpu,24u,47u,0u,0u,0u,0u);
 trapped(&cpu,47u);
 assert(gpu.load_matrix==1u&&gpu.mult_matrix==1u);
 gpu.reject=1;
 api(&cpu,24u,47u,matrix_ptr,0u,0u,0u);
 trapped(&cpu,47u); /* not a fake success on GPU error */
 gpu.reject=0;
 assert(gpu.load_matrix==1u);

 /* Fifth and sixth glFrustumx and glOrthox arguments use guest SP.
  * Bad guest SP must fail before a projection callback is invoked. */
 const uint32_t sp=cpu.regs[13];
 const uint32_t stackoff=sp-CNK_ARM_STACK_BASE;
 wr(cpu.stack+stackoff,65536u); /* near 1 */
 wr(cpu.stack+stackoff+4u,1048576u); /* far 16 */
 api(&cpu,24u,35u,(uint32_t)-65536,65536u,
     (uint32_t)-32768,32768u);
 assert(success(&cpu)==0u&&gpu.frustum==1u);
 assert(gpu.projection[0]==-1.f&&gpu.projection[1]==1.f);
 assert(gpu.projection[2]==-0.5f&&gpu.projection[3]==0.5f);
 assert(gpu.projection[4]==1.f&&gpu.projection[5]==16.f);

 wr(cpu.stack+stackoff,(uint32_t)-65536); /* near -1 */
 wr(cpu.stack+stackoff+4u,65536u); /* far 1 */
 api(&cpu,24u,56u,0u,640u*65536u,0u,480u*65536u);
 assert(success(&cpu)==0u&&gpu.ortho==1u);
 assert(gpu.projection[0]==0.f&&gpu.projection[1]==640.f);
 assert(gpu.projection[3]==480.f&&gpu.projection[4]==-1.f);
 assert(gpu.projection[5]==1.f);
 api(&cpu,24u,35u,(uint32_t)-65536,65536u,
     (uint32_t)-32768,32768u);
 trapped(&cpu,35u); /* negative near is invalid for perspective */
 assert(gpu.frustum==1u);
 cpu.regs[13]=CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-4u;
 api(&cpu,24u,56u,0u,65536u,0u,65536u);
 trapped(&cpu,56u); /* sixth stack argument outside guest memory */
 cpu.regs[13]=sp+1u;
 api(&cpu,24u,35u,(uint32_t)-65536,65536u,0u,65536u);
 trapped(&cpu,35u); /* misaligned guest SP */
 cpu.regs[13]=sp;
 api(&cpu,24u,56u,65536u,65536u,0u,65536u);
 trapped(&cpu,56u); /* zero projection width */
 assert(gpu.ortho==1u);
 /* Unbound optional callbacks must trap, not produce empty frames. */
 cpu.gl_host.ortho=NULL;
 api(&cpu,24u,56u,0u,65536u,0u,65536u);
 trapped(&cpu,56u);
 cpu.gl_host.ortho=ortho;
 gpu.reject=1;
 api(&cpu,24u,56u,0u,65536u,0u,65536u);
 trapped(&cpu,56u);
 assert(gpu.ortho==1u);
 gpu.reject=0;
 api(&cpu,24u,56u,0u,65536u,0u,65536u);
 assert(success(&cpu)==0u&&gpu.ortho==2u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: Zeebx original GL fixed-point matrices, orthographic/"
      "perspective projection, ARM stack arguments and GPU failures");
 return 0;
}
