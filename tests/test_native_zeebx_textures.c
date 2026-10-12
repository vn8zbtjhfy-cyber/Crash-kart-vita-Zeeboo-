/* ZeebxTeam/zeebx-emu original GL_LEGACY texture slots from aee_slots.rs.
 * Synthetic ARM guest state only; no game files, ROM or fake GPU success.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 unsigned gen,bind,upload,parameters,deleted,draw,plain,cap;
 uint32_t last_texture,last_format,last_type,last_w,last_h,last_parameter;
 float vertex[9],uv[6];
 uint8_t pixels[16];
 int failure;
} gpu_recorder;
static int ok_start(void *u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int ok_present(void *u){(void)u;return 0;}
static int ok_clear(void *u,uint32_t m){(void)u;(void)m;return 0;}
static int ok_color(void *u,float a,float b,float c,float d){
 (void)u;(void)a;(void)b;(void)c;(void)d;return 0;
}
static int ok_view(void *u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int generate(void *u,uint32_t n,uint32_t *names){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure||n>32u)return -1;
 for(uint32_t i=0;i<n;i++)names[i]=100u+i;
 g->gen++;return 0;
}
static int binding(void *u,uint32_t n){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure)return -1;
 g->bind++;g->last_texture=n;return 0;
}
static int remove_names(void *u,uint32_t n,const uint32_t *names){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure||n>32u||!names)return -1;
 g->deleted++;
 return 0;
}
static int params(void *u,uint32_t pname,uint32_t value){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure)return -1;
 g->parameters++;g->last_parameter=value;
 return pname==0x2801u?0:-1;
}
static int uploaded(void *u,uint32_t w,uint32_t h,uint32_t f,
                    uint32_t type,const uint8_t *data){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure||!data)return -1;
 g->upload++;g->last_w=w;g->last_h=h;g->last_format=f;
 g->last_type=type;
 const unsigned len=w*h*(f==0x1908u?4u:2u);
 if(len<=sizeof(g->pixels))memcpy(g->pixels,data,len);
 return 0;
}
static int capability(void *u,uint32_t cap,int enabled){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure || cap!=0x0de1u)return -1;
 g->cap++;return enabled==0||enabled==1?0:-1;
}
static int textured(void *u,const float *xyz,const float *uv,uint32_t n){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure||n!=3u)return -1;
 g->draw++;
 memcpy(g->vertex,xyz,sizeof(g->vertex));
 memcpy(g->uv,uv,sizeof(g->uv));
 return 0;
}
static int untextured(void *u,const float *xyz,uint32_t n){
 gpu_recorder *g=(gpu_recorder*)u;
 if(g->failure||!xyz||n!=3u)return -1;
 g->plain++;return 0;
}
static void api(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                uint32_t a,uint32_t b,uint32_t d,uint32_t e){
 c->regs[0]=a;c->regs[1]=b;c->regs[2]=d;c->regs[3]=e;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t success(cnk_zeebx_arm *c){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 assert(o.reason==CNK_ARM_RETURNED && handled==1u);
 return c->regs[0];
}
static void stopped(cnk_zeebx_arm *c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 assert(o.reason==CNK_ARM_API && handled==0u);
 assert(o.api_interface==24u&&o.api_slot==slot);
}
static void wr32(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
static void guest_float(cnk_zeebx_arm *c,uint32_t p,float v){
 uint32_t bits=0;memcpy(&bits,&v,sizeof(v));
 wr32(c->heap+(p-CNK_ARM_HEAP_BASE),bits);
}
static void stack_params(cnk_zeebx_arm *c,uint32_t h,uint32_t border,
                         uint32_t format,uint32_t type,uint32_t pixels){
 const uint32_t at=c->regs[13]-CNK_ARM_STACK_BASE;
 wr32(c->stack+at,h);wr32(c->stack+at+4u,border);
 wr32(c->stack+at+8u,format);wr32(c->stack+at+12u,type);
 wr32(c->stack+at+16u,pixels);
}
int main(void){
 uint8_t mod[128]={0};
 cnk_zeebx_arm c={0};gpu_recorder recorder={0};
 const cnk_zeebx_gl_host host={
  .make_current=ok_start,.present=ok_present,.clear=ok_clear,
  .clear_color=ok_color,.viewport=ok_view,
  .texture_gen=generate,.texture_bind=binding,
  .texture_delete=remove_names,.texture_parameter=params,
  .texture_upload=uploaded,.set_capability=capability,
  .draw_triangles=untextured,.draw_textured_triangles=textured
 };
 assert(!cnk_zeebx_arm_init(&c,mod,sizeof(mod)));
 /* Nine-argument TexImage2D needs five words at SP; the boot entry SP
  * reserves only sixteen bytes. Place this synthetic call's frame safely. */
 c.regs[13]=CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-64u;
 assert(!cnk_zeebx_arm_set_gl_host(&c,&host,&recorder));
 api(&c,23u,5u,1u,0u,0u,0u);assert(success(&c)==1u);
 api(&c,23u,12u,1u,1u,0u,0u);const uint32_t surf=success(&c);
 api(&c,23u,17u,1u,1u,0u,0u);const uint32_t ctx=success(&c);
 api(&c,23u,19u,1u,surf,surf,ctx);assert(success(&c)==1u);

 const uint32_t out=CNK_ARM_HEAP_BASE+0x40u;
 api(&c,24u,36u,2u,out,0u,0u);assert(success(&c)==0u);
 uint32_t n0=0,n1=0;
 assert(!cnk_zeebx_arm_read_u32(&c,out,&n0)&&n0==100u);
 assert(!cnk_zeebx_arm_read_u32(&c,out+4u,&n1)&&n1==101u);
 assert(recorder.gen==1u);
 /* Allocating handles cannot write beyond mapped guest memory. */
 api(&c,24u,36u,2u,CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-4u,0u,0u);
 stopped(&c,36u);assert(recorder.gen==1u);
 api(&c,24u,36u,33u,out,0u,0u);stopped(&c,36u);
 api(&c,24u,5u,0x0de1u,n0,0u,0u);assert(success(&c)==0u);
 assert(c.gl_bound_texture==100u&&recorder.bind==1u);
 api(&c,24u,75u,0x0de1u,0x2801u,0x2601u,0u);
 assert(success(&c)==0u && recorder.parameters==1u);
 api(&c,24u,75u,0x0de1u,0x2801u,0xdeadbeefu,0u);
 stopped(&c,75u);
 assert(recorder.parameters==1u);

 const uint32_t tex=CNK_ARM_HEAP_BASE+0x100u;
 for(unsigned i=0;i<16u;i++)c.heap[0x100u+i]=(uint8_t)(i*7u);
 stack_params(&c,2u,0u,0x1908u,0x1401u,tex);
 api(&c,24u,74u,0x0de1u,0u,0x1908u,2u);
 assert(success(&c)==0u);
 assert(recorder.upload==1u&&c.gl_texture_uploads==1u);
 assert(recorder.last_w==2u&&recorder.last_h==2u);
 assert(recorder.last_format==0x1908u&&recorder.last_type==0x1401u);
 for(unsigned i=0;i<16u;i++)assert(recorder.pixels[i]==i*7u);

 /* RGB565 source bytes (not invented RGBA remapping). */
 c.heap[0x120u]=0x00u;c.heap[0x121u]=0xf8u;
 stack_params(&c,1u,0u,0x1907u,0x8363u,tex+0x20u);
 api(&c,24u,74u,0x0de1u,0u,0x1907u,1u);
 assert(success(&c)==0u&&recorder.upload==2u);
 assert(recorder.last_format==0x1907u&&recorder.last_type==0x8363u);
 assert(recorder.pixels[0]==0x00u&&recorder.pixels[1]==0xf8u);

 stack_params(&c,1024u,0u,0x1908u,0x1401u,tex);
 api(&c,24u,74u,0x0de1u,0u,0x1908u,1u);stopped(&c,74u);
 stack_params(&c,2u,0u,0x1908u,0x1401u,
              CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-4u);
 api(&c,24u,74u,0x0de1u,0u,0x1908u,2u);stopped(&c,74u);
 stack_params(&c,2u,1u,0x1908u,0x1401u,tex);
 api(&c,24u,74u,0x0de1u,0u,0x1908u,2u);stopped(&c,74u);
 stack_params(&c,2u,0u,0x1908u,0x1401u,tex);
 c.regs[13]+=1u;
 api(&c,24u,74u,0x0de1u,0u,0x1908u,2u);stopped(&c,74u);
 c.regs[13]-=1u;
 assert(recorder.upload==2u&&c.gl_texture_uploads==2u);
 /* Host GPU rejection is propagated, not reported as an upload. */
 recorder.failure=1;
 api(&c,24u,74u,0x0de1u,0u,0x1908u,2u);stopped(&c,74u);
 assert(recorder.upload==2u&&c.gl_texture_uploads==2u);
 recorder.failure=0;

 const uint32_t xyz=CNK_ARM_HEAP_BASE+0x200u;
 const uint32_t uv=CNK_ARM_HEAP_BASE+0x300u;
 for(unsigned i=0;i<9u;i++)guest_float(&c,xyz+4u*i,(float)i);
 for(unsigned i=0;i<6u;i++)guest_float(&c,uv+4u*i,(float)i*0.25f);
 api(&c,24u,78u,3u,0x1406u,0u,xyz);assert(success(&c)==0u);
 api(&c,24u,29u,0x8074u,0u,0u,0u);assert(success(&c)==0u);
 api(&c,24u,71u,2u,0x1406u,0u,uv);assert(success(&c)==0u);
 api(&c,24u,29u,0x8078u,0u,0u,0u);assert(success(&c)==0u);
 api(&c,24u,28u,0x0de1u,0u,0u,0u);assert(success(&c)==0u);
 assert(c.gl_texturing_enabled&&c.gl_uv_enabled);
 api(&c,24u,26u,4u,0u,3u,0u);assert(success(&c)==0u);
 assert(recorder.draw==1u&&recorder.plain==0u);
 assert(c.gl_draw_calls==1u&&c.gl_textured_draw_calls==1u);
 for(unsigned i=0;i<9u;i++)assert(recorder.vertex[i]==(float)i);
 for(unsigned i=0;i<6u;i++)
  assert(recorder.uv[i]==(float)i*0.25f);

 /* Disabling UV data prevents texturing, rather than using stale values. */
 api(&c,24u,25u,0x8078u,0u,0u,0u);assert(success(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 api(&c,24u,29u,0x8078u,0u,0u,0u);assert(success(&c)==0u);
 guest_float(&c,uv+4u,__builtin_nanf(""));
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 guest_float(&c,uv+4u,0.25f);
 c.gl_uv_pointer=CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-4u;
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 c.gl_uv_pointer=uv;
 recorder.failure=1;
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 recorder.failure=0;
 assert(recorder.draw==1u&&c.gl_textured_draw_calls==1u);
 /* Plain triangles still use the older proven rendering path. */
 api(&c,24u,23u,0x0de1u,0u,0u,0u);assert(success(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);assert(success(&c)==0u);
 assert(recorder.plain==1u&&recorder.draw==1u);
 api(&c,24u,20u,1u,out,0u,0u);assert(success(&c)==0u);
 assert(recorder.deleted==1u&&c.gl_bound_texture==0u);
 api(&c,24u,20u,1u,CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-2u,0u,0u);
 stopped(&c,20u);
 assert(recorder.deleted==1u);

 cnk_zeebx_arm_destroy(&c);
 puts("PASS: original Zeebx texture slots forward guest RGBA/RGB565,"
      " GLES1 texture IDs/UV, and invalid pointers/GL failures trap");
 return 0;
}
