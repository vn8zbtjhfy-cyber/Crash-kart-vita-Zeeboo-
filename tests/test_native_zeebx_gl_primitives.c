/* Source-backed GL_LEGACY draw modes: triangles, strips, fans.
 * Pure synthetic ARM guest coordinates/indices and a recorder backend.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 uint32_t draws,vertices;
 float xyz[CNK_ZEEBX_MAX_DRAW_VERTICES*3u];
 int reject;
} recorder;
static int begin(void *p,uint32_t w,uint32_t h){
 (void)p;return w==640u&&h==480u?0:-1;
}
static int clearcolor(void *p,float a,float b,float c,float d){
 (void)p;(void)a;(void)b;(void)c;(void)d;return 0;
}
static int clear(void *p,uint32_t m){(void)p;(void)m;return 0;}
static int viewport(void *p,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)p;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int present(void *p){(void)p;return 0;}
static int draw(void *p,const float *xyz,uint32_t count){
 recorder *r=(recorder*)p;
 if(r->reject||!xyz||!count||count%3u||
    count>CNK_ZEEBX_MAX_DRAW_VERTICES)return -1;
 r->draws++;r->vertices=count;
 memcpy(r->xyz,xyz,3u*count*sizeof(float));
 return 0;
}
static void wr32(uint8_t *p,uint32_t x){
 for(unsigned j=0;j<4u;j++)p[j]=(uint8_t)(x>>(8u*j));
}
static void wr_float(uint8_t *p,float value){
 uint32_t x=0;memcpy(&x,&value,4u);wr32(p,x);
}
static void api(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                uint32_t r0,uint32_t r1,uint32_t r2,uint32_t r3){
 c->regs[0]=r0;c->regs[1]=r1;c->regs[2]=r2;c->regs[3]=r3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12u)+slot*4u;
}
static uint32_t success(cnk_zeebx_arm *c){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,30u,&o,&n));
 assert(o.reason==CNK_ARM_RETURNED&&n==1u);
 return c->regs[0];
}
static void stopped(cnk_zeebx_arm *c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,30u,&o,&n));
 assert(o.reason==CNK_ARM_API&&o.api_interface==24u&&
        o.api_slot==slot&&n==0u);
}
static void check(recorder *r,const float *vertices,
                  const unsigned *indices,unsigned count){
 assert(r->vertices==count);
 for(unsigned i=0;i<count;i++)for(unsigned a=0;a<3u;a++)
  assert(r->xyz[i*3u+a]==vertices[indices[i]*3u+a]);
}
int main(void){
 uint8_t guest[256]={0};cnk_zeebx_arm cpu={0};
 recorder gpu={0};
 const cnk_zeebx_gl_host host={
  .make_current=begin,.clear_color=clearcolor,.clear=clear,
  .viewport=viewport,.present=present,.draw_triangles=draw
 };
 assert(!cnk_zeebx_arm_init(&cpu,guest,sizeof(guest)));
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&host,&gpu));
 api(&cpu,23u,5u,1u,0u,0u,0u);assert(success(&cpu)==1u);
 api(&cpu,23u,12u,1u,1u,0u,0u);
 const uint32_t surface=success(&cpu);
 api(&cpu,23u,17u,1u,1u,0u,0u);
 const uint32_t context=success(&cpu);
 api(&cpu,23u,19u,1u,surface,surface,context);
 assert(success(&cpu)==1u);

 const uint32_t vptr=CNK_ARM_HEAP_BASE+256u;
 const uint32_t iptr=CNK_ARM_HEAP_BASE+1024u;
 const float vertices[12]={
  0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f,1.f,1.f,0.f
 };
 for(unsigned i=0;i<12u;i++)wr_float(cpu.heap+256u+i*4u,vertices[i]);
 api(&cpu,24u,78u,3u,0x1406u,12u,vptr);
 assert(success(&cpu)==0u);
 api(&cpu,24u,29u,0x8074u,0u,0u,0u);
 assert(success(&cpu)==0u);
 /* Strip 0,1,2,3 -> 0,1,2 | 2,1,3, preserving front-face winding. */
 api(&cpu,24u,26u,5u,0u,4u,0u);
 assert(success(&cpu)==0u&&gpu.draws==1u);
 const unsigned strip[6]={0u,1u,2u,2u,1u,3u};
 check(&gpu,vertices,strip,6u);

 /* Fan 0,1,2,3 -> 0,1,2 | 0,2,3. */
 api(&cpu,24u,26u,6u,0u,4u,0u);
 assert(success(&cpu)==0u&&gpu.draws==2u);
 const unsigned fan[6]={0u,1u,2u,0u,2u,3u};
 check(&gpu,vertices,fan,6u);

 /* Indexed strip BYTE [3,2,1,0] uses guest index bytes as-is. */
 cpu.heap[1024u]=3u;cpu.heap[1025u]=2u;
 cpu.heap[1026u]=1u;cpu.heap[1027u]=0u;
 api(&cpu,24u,27u,5u,4u,0x1401u,iptr);
 assert(success(&cpu)==0u&&gpu.draws==3u);
 const unsigned indexed_strip[6]={3u,2u,1u,1u,2u,0u};
 check(&gpu,vertices,indexed_strip,6u);

 /* Indexed fan SHORT [2,0,3,1] and correct center index 2. */
 const uint32_t iptr16=iptr+32u;
 const unsigned short_idx[4]={2u,0u,3u,1u};
 for(unsigned i=0;i<4u;i++){
  cpu.heap[1056u+2u*i]=(uint8_t)short_idx[i];
  cpu.heap[1057u+2u*i]=0u;
 }
 api(&cpu,24u,27u,6u,4u,0x1403u,iptr16);
 assert(success(&cpu)==0u&&gpu.draws==4u);
 const unsigned indexed_fan[6]={2u,0u,3u,2u,3u,1u};
 check(&gpu,vertices,indexed_fan,6u);

 /* Original triangle mode remains byte-for-byte stable. */
 api(&cpu,24u,26u,4u,0u,3u,0u);
 assert(success(&cpu)==0u&&gpu.draws==5u);
 const unsigned original[3]={0u,1u,2u};
 check(&gpu,vertices,original,3u);
 assert(cpu.gl_draw_calls==5u&&cpu.gl_drawn_vertices==27u);

 api(&cpu,24u,26u,1u,0u,4u,0u); /* GL_LINES is not drawn */
 stopped(&cpu,26u);
 api(&cpu,24u,26u,5u,0u,2u,0u); /* incomplete strip */
 stopped(&cpu,26u);
 api(&cpu,24u,26u,6u,0u,0u,0u); /* zero-length fan */
 stopped(&cpu,26u);
 api(&cpu,24u,26u,4u,0u,4u,0u); /* incomplete triangles */
 stopped(&cpu,26u);
 api(&cpu,24u,26u,5u,0u,CNK_ZEEBX_MAX_DRAW_VERTICES/3u+3u,0u);
 stopped(&cpu,26u); /* would exceed bounded expanded output */
 api(&cpu,24u,26u,5u,65535u,4u,0u);
 stopped(&cpu,26u); /* first+count out of guest range */
 api(&cpu,24u,27u,6u,4u,0x1403u,iptr16+1u);
 stopped(&cpu,27u); /* invalid u16 index alignment */
 api(&cpu,24u,27u,5u,4u,0x1401u,
     CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-2u);
 stopped(&cpu,27u); /* four bytes unavailable, no partial draw */

 /* Last fan vertex is corrupted: no draw callback with partial output. */
 wr_float(cpu.heap+256u+3u*3u*4u,NAN);
 api(&cpu,24u,26u,6u,0u,4u,0u);
 stopped(&cpu,26u);
 assert(gpu.draws==5u);
 wr_float(cpu.heap+256u+3u*3u*4u,1.f);
 gpu.reject=1;
 api(&cpu,24u,26u,5u,0u,4u,0u);
 stopped(&cpu,26u);
 assert(gpu.draws==5u&&cpu.gl_draw_calls==5u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS: original Zeebo GL indexed/nonindexed triangles, strip/fan"
      " winding, bounded guest buffers and no partial GPU draws");
 return 0;
}
