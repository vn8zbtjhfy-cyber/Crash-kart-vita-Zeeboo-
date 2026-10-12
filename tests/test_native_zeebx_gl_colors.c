/* Original Zeebx BREW legacy glColorPointer GL_UNSIGNED_BYTE RGBA:
 * synthetic memory / VBO and indexed geometry. No proprietary assets. */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct{unsigned plain,colored,colored_tex;int reject;float colors[12];} recorder;
static int begin(void*u,uint32_t w,uint32_t h){(void)u;return w==640u&&h==480u?0:-1;}
static int noop_color(void*u,float a,float b,float c,float d){(void)u;(void)a;(void)b;(void)c;(void)d;return 0;}
static int noop_clear(void*u,uint32_t mask){(void)u;(void)mask;return 0;}
static int noop_view(void*u,int32_t x,int32_t y,uint32_t w,uint32_t h){(void)u;(void)x;(void)y;(void)w;(void)h;return 0;}
static int noop_present(void*u){(void)u;return 0;}
static int cap(void*u,uint32_t n,int e){(void)u;return n==0x0de1u&&(e==0||e==1)?0:-1;}
static int bind_tex(void*u,uint32_t name){(void)u;return name==81u?0:-1;}
static int upload(void*u,uint32_t w,uint32_t h,uint32_t f,uint32_t t,const uint8_t*p){(void)u;(void)w;(void)h;(void)f;(void)t;(void)p;return 0;}
static int plain(void*u,const float*xyz,uint32_t n){
 recorder*r=(recorder*)u;if(r->reject||!xyz||n!=3u)return -1;
 r->plain++;return 0;
}
static int colored(void*u,const float*xyz,const float*rgba,uint32_t n){
 recorder*r=(recorder*)u;if(r->reject||!xyz||!rgba||n!=3u)return -1;
 memcpy(r->colors,rgba,sizeof(r->colors));r->colored++;return 0;
}
static int colored_tex(void*u,const float*xyz,const float*uv,const float*rgba,uint32_t n){
 recorder*r=(recorder*)u;if(r->reject||!xyz||!uv||!rgba||n!=3u)return -1;
 memcpy(r->colors,rgba,sizeof(r->colors));r->colored_tex++;return 0;
}
static void wr16(uint8_t *p,int16_t v){
 uint16_t x=(uint16_t)v;p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static void call(cnk_zeebx_arm*c,unsigned iface,unsigned slot,uint32_t a,uint32_t b,uint32_t d,uint32_t e){
 c->regs[0]=a;c->regs[1]=b;c->regs[2]=d;c->regs[3]=e;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t ok(cnk_zeebx_arm*c){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 if(o.reason!=CNK_ARM_RETURNED||handled!=1u)
  fprintf(stderr,"Zeebx COLOR iface %u slot %u reason %u\n",
          o.api_interface,o.api_slot,(unsigned)o.reason);
 assert(o.reason==CNK_ARM_RETURNED&&handled==1u);
 return c->regs[0];
}
static void trapped(cnk_zeebx_arm*c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 assert(o.reason==CNK_ARM_API&&handled==0u&&o.api_slot==slot);
}
static void verify(const recorder*r,const uint8_t samples[3][4],int reverse){
 for(unsigned i=0;i<3u;i++)for(unsigned ch=0;ch<4u;ch++){
  unsigned j=reverse?2u-i:i;
  assert(r->colors[i*4u+ch]==(float)samples[j][ch]/255.f);
 }
}
int main(void){
 uint8_t mod[128]={0};cnk_zeebx_arm c={0};recorder r={0};
 const cnk_zeebx_gl_host host={
  .make_current=begin,.clear_color=noop_color,.clear=noop_clear,
  .viewport=noop_view,.present=noop_present,.set_capability=cap,
  .texture_bind=bind_tex,.texture_upload=upload,
  .draw_triangles=plain,.draw_colored_triangles=colored,
  .draw_colored_textured_triangles=colored_tex
 };
 assert(!cnk_zeebx_arm_init(&c,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_gl_host(&c,&host,&r));
 call(&c,23u,5u,1u,0u,0u,0u);assert(ok(&c)==1u);
 call(&c,23u,12u,1u,1u,0u,0u);const uint32_t surf=ok(&c);
 call(&c,23u,17u,1u,1u,0u,0u);const uint32_t ctx=ok(&c);
 call(&c,23u,19u,1u,surf,surf,ctx);assert(ok(&c)==1u);
 const uint32_t xyz=CNK_ARM_HEAP_BASE+0x100u;
 const uint32_t uv=CNK_ARM_HEAP_BASE+0x200u;
 const uint32_t colors=CNK_ARM_HEAP_BASE+0x300u;
 const uint8_t samples[3][4]={{255,0,0,255},{0,255,0,128},{32,0,255,200}};
 for(unsigned i=0;i<3u;i++){
  for(unsigned j=0;j<3u;j++)wr16(c.heap+0x100u+i*6u+j*2u,(int16_t)(i*3u+j));
  for(unsigned j=0;j<2u;j++)wr16(c.heap+0x200u+i*4u+j*2u,(int16_t)(i*2u+j));
  memcpy(c.heap+0x300u+i*8u,samples[i],4u);
  memset(c.heap+0x300u+i*8u+4u,0xee,4u);
 }
 call(&c,24u,78u,3u,0x1402u,0u,xyz);assert(ok(&c)==0u);
 call(&c,24u,29u,0x8074u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.plain==1u && r.colored==0u);
 call(&c,24u,14u,4u,0x1401u,8u,colors);assert(ok(&c)==0u);
 call(&c,24u,29u,0x8076u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.colored==1u);verify(&r,samples,0);
 const uint32_t index=CNK_ARM_HEAP_BASE+0x450u;
 for(unsigned i=0;i<3u;i++)wr16(c.heap+0x450u+i*2u,(int16_t)(2u-i));
 call(&c,24u,27u,4u,3u,0x1403u,index);assert(ok(&c)==0u);
 assert(r.colored==2u);verify(&r,samples,1);
 call(&c,24u,71u,2u,0x1402u,0u,uv);assert(ok(&c)==0u);
 call(&c,24u,29u,0x8078u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,5u,0x0de1u,81u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,28u,0x0de1u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.colored_tex==1u);verify(&r,samples,0);
 /* VBO byte pointer and binding snapshot; compact colors in other buffer. */
 const uint32_t names=CNK_ARM_HEAP_BASE+0x500u;
 call(&c,24u,128u,1u,names,0u,0u);assert(ok(&c)==0u);
 uint32_t vbo=0;assert(!cnk_zeebx_arm_read_u32(&c,names,&vbo));
 call(&c,24u,119u,0x8892u,vbo,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,120u,0x8892u,64u,0u,0x88e4u);assert(ok(&c)==0u);
 for(unsigned i=0;i<3u;i++)memcpy(c.heap+0x600u+i*4u,samples[i],4u);
 call(&c,24u,121u,0x8892u,6u,12u,CNK_ARM_HEAP_BASE+0x600u);
 assert(ok(&c)==0u);
 call(&c,24u,14u,4u,0x1401u,0u,6u);assert(ok(&c)==0u);
 assert(c.gl_color_buffer==vbo&&c.gl_color_pointer==6u&&
        c.gl_color_stride==4u);
 call(&c,24u,119u,0x8892u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,27u,4u,3u,0x1403u,index);assert(ok(&c)==0u);
 assert(r.colored_tex==2u);verify(&r,samples,1);
 const unsigned old=r.colored_tex;
 /* No phantom success for absent host callback, truncated VBO or GPU failure. */
 c.gl_color_pointer=62u;
 call(&c,24u,26u,4u,0u,3u,0u);trapped(&c,26u);
 assert(r.colored_tex==old);
 c.gl_color_pointer=6u;
 c.gl_host.draw_colored_textured_triangles=NULL;
 call(&c,24u,26u,4u,0u,3u,0u);trapped(&c,26u);
 assert(r.colored_tex==old);
 c.gl_host.draw_colored_textured_triangles=colored_tex;
 r.reject=1;
 call(&c,24u,26u,4u,0u,3u,0u);trapped(&c,26u);
 assert(r.colored_tex==old);
 r.reject=0;
 call(&c,24u,14u,4u,0x1402u,8u,colors);trapped(&c,14u);
 assert(c.gl_color_type==0x1401u);
 call(&c,24u,25u,0x8076u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,24u,0x0de1u,0u,0u,0u);assert(ok(&c)==0u);
 call(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.plain==2u);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx original color arrays UBYTE RGBA + VBO, indexed and"
      " textured triangles, normalized channels and GPU safety traps");
 return 0;
}
