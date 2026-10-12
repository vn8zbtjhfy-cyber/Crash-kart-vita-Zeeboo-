/* Original Zeebx GL_FIXED 16.16 arrays to host vitaGL float callbacks.
 * Synthetic ROM-free BREW ARM guest memory, never commercial game assets.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 unsigned draws;
 int reject;
 float xyz[9],uv[6];
} recorder;
static int start(void*u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int col(void*u,float a,float b,float c,float d){
 (void)u;(void)a;(void)b;(void)c;(void)d;return 0;
}
static int clear(void*u,uint32_t m){(void)u;(void)m;return 0;}
static int view(void*u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int present(void*u){(void)u;return 0;}
static int cap(void*u,uint32_t mode,int on){
 (void)u;return mode==0x0de1u&&(on==0||on==1)?0:-1;
}
static int bind_tex(void*u,uint32_t name){
 (void)u;return name==99u?0:-1;
}
static int upload(void*u,uint32_t w,uint32_t h,uint32_t f,
                  uint32_t type,const uint8_t*px){
 (void)u;(void)w;(void)h;(void)f;(void)type;(void)px;return 0;
}
static int draw(void*u,const float*xyz,const float*uv,uint32_t n){
 recorder*g=(recorder*)u;
 if(g->reject||n!=3u||!xyz||!uv)return -1;
 memcpy(g->xyz,xyz,sizeof(g->xyz));
 memcpy(g->uv,uv,sizeof(g->uv));
 g->draws++;
 return 0;
}
static void wr32(uint8_t*p,uint32_t x){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(i*8u));
}
static void fixed(cnk_zeebx_arm*c,uint32_t address,int32_t value){
 assert(address>=CNK_ARM_HEAP_BASE);
 wr32(c->heap+address-CNK_ARM_HEAP_BASE,(uint32_t)value);
}
static void real(cnk_zeebx_arm*c,uint32_t address,float value){
 uint32_t bits=0;memcpy(&bits,&value,4u);
 fixed(c,address,(int32_t)bits);
}
static void call(cnk_zeebx_arm*c,unsigned iface,unsigned slot,
                 uint32_t r0,uint32_t r1,uint32_t r2,uint32_t r3){
 c->regs[0]=r0;c->regs[1]=r1;c->regs[2]=r2;c->regs[3]=r3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+4u*slot;
}
static uint32_t ok(cnk_zeebx_arm*c){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&n));
 assert(o.reason==CNK_ARM_RETURNED&&n==1u);
 return c->regs[0];
}
static void stopped(cnk_zeebx_arm*c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&n));
 assert(o.reason==CNK_ARM_API&&o.api_slot==slot&&n==0u);
}
int main(void){
 uint8_t mod[128]={0};
 cnk_zeebx_arm c={0};recorder rec={0};
 const cnk_zeebx_gl_host gpu={
  .make_current=start,.clear_color=col,.clear=clear,.viewport=view,
  .present=present,.set_capability=cap,.texture_bind=bind_tex,
  .texture_upload=upload,.draw_textured_triangles=draw
 };
 assert(!cnk_zeebx_arm_init(&c,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_gl_host(&c,&gpu,&rec));
 call(&c,23u,5u,1u,0u,0u,0u);assert(ok(&c)==1u);
 call(&c,23u,12u,1u,1u,0u,0u);uint32_t surf=ok(&c);
 call(&c,23u,17u,1u,1u,0u,0u);uint32_t ctx=ok(&c);
 call(&c,23u,19u,1u,surf,surf,ctx);assert(ok(&c)==1u);
 call(&c,24u,5u,0x0de1u,99u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,28u,0x0de1u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,29u,0x8074u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,29u,0x8078u,0u,0u,0u);assert(ok(&c)==0u);

 const uint32_t xyz=CNK_ARM_HEAP_BASE+0x100u;
 const uint32_t uv=CNK_ARM_HEAP_BASE+0x400u;
 /* 4 signed fixed values per vertex: 3 XYZ plus interleaved padding. */
 const int32_t original[9]={
  -65536,0,0, 98304,0,0, 0,147456,-32768
 };
 const int32_t coords[6]={0,0, 65536,0, 32768,65536};
 for(unsigned i=0;i<3u;i++){
  for(unsigned j=0;j<3u;j++)
   fixed(&c,xyz+16u*i+4u*j,original[3u*i+j]);
  fixed(&c,xyz+16u*i+12u,0x12345678);
  for(unsigned j=0;j<2u;j++)
   fixed(&c,uv+12u*i+4u*j,coords[2u*i+j]);
  fixed(&c,uv+12u*i+8u,0x12345678);
 }
 call(&c,24u,78u,3u,0x140cu,16u,xyz);assert(ok(&c)==0u);
 call(&c,24u,71u,2u,0x140cu,12u,uv);assert(ok(&c)==0u);
 assert(c.gl_vertex_type==0x140cu&&c.gl_uv_type==0x140cu);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(rec.draws==1u && c.gl_draw_calls==1u &&
        c.gl_textured_draw_calls==1u);
 for(unsigned i=0;i<9u;i++)assert(rec.xyz[i]==(float)original[i]/65536.f);
 for(unsigned i=0;i<6u;i++)assert(rec.uv[i]==(float)coords[i]/65536.f);

 /* DrawElements via original uint16 indices, retain source vertex order. */
 const uint32_t idx=CNK_ARM_HEAP_BASE+0x800u;
 c.heap[0x800u]=2u;c.heap[0x801u]=0u;
 c.heap[0x802u]=1u;c.heap[0x803u]=0u;
 c.heap[0x804u]=0u;c.heap[0x805u]=0u;
 call(&c,24u,27u,4u,3u,0x1403u,idx);assert(ok(&c)==0u);
 assert(rec.draws==2u&&c.gl_draw_calls==2u);
 for(unsigned i=0;i<3u;i++)for(unsigned j=0;j<3u;j++)
  assert(rec.xyz[3u*i+j]==(float)original[(2u-i)*3u+j]/65536.f);
 for(unsigned i=0;i<3u;i++)for(unsigned j=0;j<2u;j++)
  assert(rec.uv[2u*i+j]==(float)coords[(2u-i)*2u+j]/65536.f);

 /* Mixed source formats must be independent: fixed XYZ, float UV. */
 const float fuv[6]={0.f,0.f,1.f,0.f,0.25f,1.f};
 for(unsigned i=0;i<3u;i++)for(unsigned j=0;j<2u;j++)
  real(&c,uv+12u*i+4u*j,fuv[2u*i+j]);
 call(&c,24u,71u,2u,0x1406u,12u,uv);assert(ok(&c)==0u);
 assert(c.gl_vertex_type==0x140cu&&c.gl_uv_type==0x1406u);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 for(unsigned i=0;i<6u;i++)assert(rec.uv[i]==fuv[i]);
 assert(rec.draws==3u);

 /* An invalid type must NOT replace the previously valid pointer state. */
 call(&c,24u,71u,2u,0x1402u,12u,uv);stopped(&c,71u);
 assert(c.gl_uv_type==0x1406u&&c.gl_uv_stride==12u);
 call(&c,24u,78u,3u,0x1402u,16u,xyz);stopped(&c,78u);
 assert(c.gl_vertex_type==0x140cu&&c.gl_vertex_stride==16u);
 call(&c,24u,78u,3u,0x140cu,11u,xyz);stopped(&c,78u);
 assert(c.gl_vertex_stride==16u);
 call(&c,24u,71u,2u,0x140cu,0u,CNK_ARM_HEAP_BASE+
                                      CNK_ARM_HEAP_BYTES-4u);
 assert(ok(&c)==0u); /* pointer is valid, span is not */
 call(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 assert(rec.draws==3u&&c.gl_draw_calls==3u);
 /* GPU host rejection is a hard stop, not a successful frame. */
 call(&c,24u,71u,2u,0x1406u,12u,uv);assert(ok(&c)==0u);
 rec.reject=1;
 call(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 assert(rec.draws==3u&&c.gl_draw_calls==3u);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS Zeebx GL_FIXED XYZ/UV, interleaved arrays, indexed geometry,"
      " mixed FLOAT UV, bad guest memory, GPU errors");
 return 0;
}
