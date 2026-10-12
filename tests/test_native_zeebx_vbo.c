/* Original Zeebx GL_LEGACY buffer objects, ROM-free synthetic BREW guest.
 * BindBuffer / BufferData / BufferSubData / Gen / Delete / DrawElements.
 * VBO pointer zero is a byte OFFSET, not address zero in ARM guest RAM.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 unsigned calls;int reject;
 float xyz[9],uv[6];
} gpu_rec;
static int begin(void*u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int color(void*u,float a,float b,float c,float d){
 (void)u;(void)a;(void)b;(void)c;(void)d;return 0;
}
static int clear(void*u,uint32_t m){(void)u;(void)m;return 0;}
static int viewport(void*u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int present(void*u){(void)u;return 0;}
static int texturing(void*u,uint32_t t,int enabled){
 (void)u;return t==0x0de1u&&(enabled==1||enabled==0)?0:-1;
}
static int bind_texture(void*u,uint32_t name){
 (void)u;return name==42u?0:-1;
}
static int upload(void*u,uint32_t w,uint32_t h,uint32_t f,uint32_t t,
                  const uint8_t*data){
 (void)u;(void)w;(void)h;(void)f;(void)t;(void)data;return 0;
}
static int triangles(void*u,const float*xyz,const float*uv,uint32_t count){
 gpu_rec*g=(gpu_rec*)u;
 if(!xyz||!uv||count!=3u||g->reject)return -1;
 memcpy(g->xyz,xyz,sizeof(g->xyz));
 memcpy(g->uv,uv,sizeof(g->uv));
 g->calls++;
 return 0;
}
static void wr32(uint8_t*p,uint32_t value){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(value>>(i*8u));
}
static void api(cnk_zeebx_arm*c,unsigned slot,uint32_t r0,uint32_t r1,
                uint32_t r2,uint32_t r3){
 c->regs[0]=r0;c->regs[1]=r1;c->regs[2]=r2;c->regs[3]=r3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(24u<<12)+slot*4u;
}
static uint32_t invoke(cnk_zeebx_arm*c){
 cnk_arm_outcome o={0};uint32_t handled=0;
 const uint32_t previous=c->regs[15];
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 if(o.reason!=CNK_ARM_RETURNED||handled!=1u)
  fprintf(stderr,"Zeebx VBO failure iface=%u slot=%u reason=%u handled=%u r0=%08x\n",
          (unsigned)((previous-CNK_ARM_API_BASE)>>12),
          (unsigned)(((previous-CNK_ARM_API_BASE)&0xfffu)/4u),
          (unsigned)o.reason,handled,c->regs[0]);
 assert(o.reason==CNK_ARM_RETURNED&&handled==1u);
 return c->regs[0];
}
static void blocked(cnk_zeebx_arm*c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 assert(o.reason==CNK_ARM_API&&o.api_slot==slot&&handled==0u);
}
int main(void){
 uint8_t mod[128]={0};cnk_zeebx_arm cpu={0};gpu_rec rec={0};
 const cnk_zeebx_gl_host host={
  .make_current=begin,.clear_color=color,.clear=clear,
  .viewport=viewport,.present=present,
  .set_capability=texturing,.texture_bind=bind_texture,
  .texture_upload=upload,.draw_textured_triangles=triangles
 };
 assert(!cnk_zeebx_arm_init(&cpu,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_gl_host(&cpu,&host,&rec));
 api(&cpu,119u,0x8892u,1u,0u,0u);
 blocked(&cpu,119u); /* EGL context not yet active */
 cpu.regs[0]=1u;cpu.regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu.regs[15]=CNK_ARM_API_BASE+(23u<<12)+5u*4u;
 assert(invoke(&cpu)==1u);
 cpu.regs[0]=1u;cpu.regs[1]=1u;
 cpu.regs[15]=CNK_ARM_API_BASE+(23u<<12)+12u*4u;
 uint32_t surf=invoke(&cpu);
 cpu.regs[0]=1u;cpu.regs[1]=1u;
 cpu.regs[15]=CNK_ARM_API_BASE+(23u<<12)+17u*4u;
 uint32_t ctx=invoke(&cpu);
 cpu.regs[0]=1u;cpu.regs[1]=surf;cpu.regs[2]=surf;cpu.regs[3]=ctx;
 cpu.regs[15]=CNK_ARM_API_BASE+(23u<<12)+19u*4u;
 assert(invoke(&cpu)==1u);
 /* Zeebx GL_LEGACY upper slots from upstream aee_slots.rs. */
 const uint32_t names=CNK_ARM_HEAP_BASE+0x40u;
 api(&cpu,128u,2u,names,0u,0u);assert(invoke(&cpu)==0u);
 uint32_t vbo=0,ibo=0;
 assert(!cnk_zeebx_arm_read_u32(&cpu,names,&vbo));
 assert(!cnk_zeebx_arm_read_u32(&cpu,names+4u,&ibo));
 assert(vbo&&ibo&&vbo!=ibo);
 api(&cpu,137u,vbo,0u,0u,0u);assert(invoke(&cpu)==0u);
 /* Generated name exists but is not a buffer until its first Bind. */
 api(&cpu,119u,0x8892u,vbo,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,137u,vbo,0u,0u,0u);assert(invoke(&cpu)==1u);
 assert(cpu.gl_array_buffer==vbo);
 /* Valid VBO memory is wholly private, with 9 fixed XYZ words at offset 0,
  * and UV at offset 64; subdata fills a null-initialized BufferData. */
 const uint32_t xyz=CNK_ARM_HEAP_BASE+0x200u;
 const uint32_t uv=CNK_ARM_HEAP_BASE+0x300u;
 const int32_t coords[9]={-65536,0,0, 65536,0,0, 0,98304,-32768};
 const int32_t texcoord[6]={0,0,65536,0,32768,65536};
 for(unsigned i=0;i<9u;i++)
  wr32(cpu.heap+0x200u+4u*i,(uint32_t)coords[i]);
 for(unsigned i=0;i<6u;i++)
  wr32(cpu.heap+0x300u+4u*i,(uint32_t)texcoord[i]);
 api(&cpu,120u,0x8892u,128u,0u,0x88e4u);assert(invoke(&cpu)==0u);
 assert(cpu.gl_total_buffer_bytes==128u);
 api(&cpu,121u,0x8892u,0u,36u,xyz);assert(invoke(&cpu)==0u);
 api(&cpu,121u,0x8892u,64u,24u,uv);assert(invoke(&cpu)==0u);
 /* Register pointers while VBO is bound. Offset 0 MUST be valid. */
 api(&cpu,78u,3u,0x140cu,12u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,71u,2u,0x140cu,8u,64u);assert(invoke(&cpu)==0u);
 assert(cpu.gl_vertex_pointer==0u&&cpu.gl_vertex_buffer==vbo);
 assert(cpu.gl_uv_pointer==64u&&cpu.gl_uv_buffer==vbo);
 api(&cpu,29u,0x8074u,0u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,29u,0x8078u,0u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,5u,0x0de1u,42u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,28u,0x0de1u,0u,0u,0u);assert(invoke(&cpu)==0u);
 /* Actual original index data resides in another VBO. */
 const uint32_t guestidx=CNK_ARM_HEAP_BASE+0x500u;
 cpu.heap[0x500u]=2u;cpu.heap[0x501u]=0u;
 cpu.heap[0x502u]=1u;cpu.heap[0x503u]=0u;
 cpu.heap[0x504u]=0u;cpu.heap[0x505u]=0u;
 api(&cpu,119u,0x8893u,ibo,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,120u,0x8893u,6u,guestidx,0x88e4u);assert(invoke(&cpu)==0u);
 api(&cpu,27u,4u,3u,0x1403u,0u);assert(invoke(&cpu)==0u);
 assert(rec.calls==1u&&cpu.gl_draw_calls==1u);
 for(unsigned i=0;i<3u;i++)
  for(unsigned j=0;j<3u;j++)
   assert(rec.xyz[3u*i+j]==(float)coords[(2u-i)*3u+j]/65536.f);
 for(unsigned i=0;i<3u;i++)
  for(unsigned j=0;j<2u;j++)
   assert(rec.uv[2u*i+j]==(float)texcoord[(2u-i)*2u+j]/65536.f);

 /* Unbinding ARRAY does not change pointer's original VBO snapshot. */
 api(&cpu,119u,0x8892u,0u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,26u,4u,0u,3u,0u);assert(invoke(&cpu)==0u);
 assert(rec.calls==2u&&cpu.gl_vertex_buffer==vbo);
 /* But binding/clearing ELEMENT affects subsequent DrawElements. */
 api(&cpu,119u,0x8893u,0u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,27u,4u,3u,0x1403u,0u);blocked(&cpu,27u);
 assert(rec.calls==2u);
 api(&cpu,119u,0x8893u,ibo,0u,0u);assert(invoke(&cpu)==0u);

 /* Invalid target, excessive allocation, and overlong subdata leave
  * VBO bytes/size/accounting unchanged. */
 api(&cpu,119u,0xdeadbeefu,99u,0u,0u);blocked(&cpu,119u);
 assert(cpu.gl_array_buffer==0u && cpu.gl_element_buffer==ibo);
 api(&cpu,120u,0x8893u,CNK_ZEEBX_MAX_GL_BUFFER_BYTES+1u,
     guestidx,0x88e4u);blocked(&cpu,120u);
 assert(cpu.gl_total_buffer_bytes==134u);
 api(&cpu,121u,0x8893u,4u,4u,guestidx);blocked(&cpu,121u);
 api(&cpu,27u,4u,3u,0x1403u,0u);assert(invoke(&cpu)==0u);
 assert(rec.calls==3u);
 rec.reject=1;
 api(&cpu,26u,4u,0u,3u,0u);blocked(&cpu,26u);
 assert(rec.calls==3u&&cpu.gl_draw_calls==3u);
 rec.reject=0;
 /* Guest unbind on deletion, and dead vertex-buffer snapshots stop. */
 api(&cpu,124u,2u,names,0u,0u);assert(invoke(&cpu)==0u);
 assert(cpu.gl_total_buffer_bytes==0u&&cpu.gl_element_buffer==0u);
 api(&cpu,137u,vbo,0u,0u,0u);assert(invoke(&cpu)==0u);
 api(&cpu,26u,4u,0u,3u,0u);blocked(&cpu,26u);
 assert(rec.calls==3u);
 cnk_zeebx_arm_destroy(&cpu);
 puts("PASS Zeebx VBO/IBO: offset 0, null BufferData, SubData, GL_FIXED UV,"
      " source-stable binding, indexed triangles and bounded memory");
 return 0;
}
