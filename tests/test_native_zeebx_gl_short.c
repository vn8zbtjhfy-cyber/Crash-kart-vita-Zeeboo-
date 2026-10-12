/* Zeebx gl.rs GL_SHORT: signed LE i16 coordinates are FLOAT model
 * units (not fixed 16.16). Execute through the original BREW GL ABI.
 * Synthetic source bytes only, no original copyrighted game assets.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
 unsigned plain,textured;
 int reject;
 float xyz[9],uv[6];
} recorder;
static int start(void *u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int color(void*u,float a,float b,float c,float d){
 (void)u;(void)a;(void)b;(void)c;(void)d;return 0;
}
static int clear(void*u,uint32_t m){(void)u;(void)m;return 0;}
static int view(void*u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int present(void*u){(void)u;return 0;}
static int cap(void*u,uint32_t mode,int enabled){
 (void)u;return mode==0x0de1u&&(enabled==0||enabled==1)?0:-1;
}
static int bind_tex(void*u,uint32_t name){
 (void)u;return name==77u?0:-1;
}
static int upload(void*u,uint32_t w,uint32_t h,uint32_t f,
                  uint32_t type,const uint8_t*p){
 (void)u;(void)w;(void)h;(void)f;(void)type;(void)p;return 0;
}
static int draw(void*u,const float*xyz,uint32_t n){
 recorder*r=(recorder*)u;
 if(r->reject||n!=3u||!xyz)return -1;
 memcpy(r->xyz,xyz,sizeof(r->xyz));r->plain++;return 0;
}
static int draw_tex(void*u,const float*xyz,const float*uv,uint32_t n){
 recorder*r=(recorder*)u;
 if(r->reject||n!=3u||!xyz||!uv)return -1;
 memcpy(r->xyz,xyz,sizeof(r->xyz));
 memcpy(r->uv,uv,sizeof(r->uv));r->textured++;return 0;
}
static void write16(uint8_t *p,int16_t v){
 uint16_t x=(uint16_t)v;p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static void write32(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));
}
static void api(cnk_zeebx_arm*c,unsigned iface,unsigned slot,
                uint32_t r0,uint32_t r1,uint32_t r2,uint32_t r3){
 c->regs[0]=r0;c->regs[1]=r1;c->regs[2]=r2;c->regs[3]=r3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t ok(cnk_zeebx_arm*c){
 cnk_arm_outcome o={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&handled));
 if(o.reason!=CNK_ARM_RETURNED||handled!=1u)
  fprintf(stderr,"Unexpected GL stop at pc=%08x iface=%u slot=%u reason=%u\n",
          o.pc,o.api_interface,o.api_slot,(unsigned)o.reason);
 assert(o.reason==CNK_ARM_RETURNED&&handled==1u);
 return c->regs[0];
}
static void stopped(cnk_zeebx_arm*c,unsigned slot){
 cnk_arm_outcome o={0};uint32_t n=0;
 assert(!cnk_zeebx_arm_bootstrap(c,24u,&o,&n));
 assert(o.reason==CNK_ARM_API&&o.api_slot==slot&&n==0u);
}
static void checked(const recorder*r,const int16_t xyz[9],
                    const int16_t uv[6],const unsigned order[3]){
 for(unsigned i=0;i<3u;i++){
  const unsigned src=order[i];
  for(unsigned j=0;j<3u;j++)
   assert(r->xyz[3u*i+j]==(float)xyz[3u*src+j]);
  for(unsigned j=0;j<2u;j++)
   assert(r->uv[2u*i+j]==(float)uv[2u*src+j]);
 }
}
int main(void){
 uint8_t mod[128]={0};cnk_zeebx_arm c={0};recorder r={0};
 const cnk_zeebx_gl_host gpu={
  .make_current=start,.clear_color=color,.clear=clear,
  .viewport=view,.present=present,.set_capability=cap,
  .texture_bind=bind_tex,.texture_upload=upload,
  .draw_triangles=draw,.draw_textured_triangles=draw_tex
 };
 assert(!cnk_zeebx_arm_init(&c,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_gl_host(&c,&gpu,&r));
 api(&c,23u,5u,1u,0u,0u,0u);assert(ok(&c)==1u);
 api(&c,23u,12u,1u,1u,0u,0u);uint32_t surf=ok(&c);
 api(&c,23u,17u,1u,1u,0u,0u);uint32_t ctx=ok(&c);
 api(&c,23u,19u,1u,surf,surf,ctx);assert(ok(&c)==1u);
 api(&c,24u,29u,0x8074u,0u,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,29u,0x8078u,0u,0u,0u);assert(ok(&c)==0u);
 const uint32_t vertex=CNK_ARM_HEAP_BASE+0x100u;
 const uint32_t texture=CNK_ARM_HEAP_BASE+0x200u;
 const int16_t xyz[9]={-32768,5,-7, 32767,12,0, 0,-123,2048};
 const int16_t uv[6]={-2,0, 512,12, 4096,-16};
 const unsigned forward[3]={0u,1u,2u},reversed[3]={2u,1u,0u};
 for(unsigned i=0;i<3u;i++){
  for(unsigned k=0;k<3u;k++)
   write16(c.heap+0x100u+i*8u+k*2u,xyz[i*3u+k]);
  write16(c.heap+0x100u+i*8u+6u,0x7777); /* stride padding */
  for(unsigned k=0;k<2u;k++)
   write16(c.heap+0x200u+i*6u+k*2u,uv[i*2u+k]);
  write16(c.heap+0x200u+i*6u+4u,0x6666);
 }
 api(&c,24u,78u,3u,0x1402u,8u,vertex);assert(ok(&c)==0u);
 api(&c,24u,71u,2u,0x1402u,6u,texture);assert(ok(&c)==0u);
 assert(c.gl_vertex_stride==8u&&c.gl_uv_stride==6u);
 api(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.plain==1u&&r.textured==0u);
 for(unsigned i=0;i<9u;i++)assert(r.xyz[i]==(float)xyz[i]);
 api(&c,24u,5u,0x0de1u,77u,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,28u,0x0de1u,0u,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 assert(r.textured==1u);
 checked(&r,xyz,uv,forward);
 /* Signed 16-bit input is NOT divided by 65536, unlike GL_FIXED. */
 assert(r.xyz[0]==-32768.f&&r.xyz[3]==32767.f);
 assert(r.uv[2]==512.f&&r.uv[5]==-16.f);
 const uint32_t idx=CNK_ARM_HEAP_BASE+0x300u;
 for(unsigned i=0;i<3u;i++)
  write16(c.heap+0x300u+2u*i,(int16_t)reversed[i]);
 api(&c,24u,27u,4u,3u,0x1403u,idx);assert(ok(&c)==0u);
 assert(r.textured==2u);
 checked(&r,xyz,uv,reversed);

 /* Default stride 0 means tightly packed six / four guest bytes. */
 const uint32_t tight=CNK_ARM_HEAP_BASE+0x400u;
 const uint32_t tight_uv=CNK_ARM_HEAP_BASE+0x500u;
 for(unsigned i=0;i<9u;i++)write16(c.heap+0x400u+i*2u,xyz[i]);
 for(unsigned i=0;i<6u;i++)write16(c.heap+0x500u+i*2u,uv[i]);
 api(&c,24u,78u,3u,0x1402u,0u,tight);assert(ok(&c)==0u);
 api(&c,24u,71u,2u,0x1402u,0u,tight_uv);assert(ok(&c)==0u);
 assert(c.gl_vertex_stride==6u&&c.gl_uv_stride==4u);
 api(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 checked(&r,xyz,uv,forward);

 /* VBO/IBO offsets are relative to THEIR OWN byte arrays.
  * Both short offsets are deliberately nonzero and non-4-aligned. */
 const uint32_t names=CNK_ARM_HEAP_BASE+0x600u;
 api(&c,24u,128u,2u,names,0u,0u);assert(ok(&c)==0u);
 uint32_t vbo=0,ibo=0;
 assert(!cnk_zeebx_arm_read_u32(&c,names,&vbo));
 assert(!cnk_zeebx_arm_read_u32(&c,names+4u,&ibo));
 assert(vbo&&ibo&&vbo!=ibo);
 api(&c,24u,119u,0x8892u,vbo,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,120u,0x8892u,128u,0u,0x88e4u);assert(ok(&c)==0u);
 api(&c,24u,121u,0x8892u,10u,24u,vertex);assert(ok(&c)==0u);
 api(&c,24u,121u,0x8892u,70u,18u,texture);assert(ok(&c)==0u);
 api(&c,24u,78u,3u,0x1402u,8u,10u);assert(ok(&c)==0u);
 api(&c,24u,71u,2u,0x1402u,6u,70u);assert(ok(&c)==0u);
 assert(c.gl_vertex_buffer==vbo&&c.gl_uv_buffer==vbo);
 api(&c,24u,119u,0x8892u,0u,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);assert(ok(&c)==0u);
 checked(&r,xyz,uv,forward); /* VBO binding snapshot survives unbind */
 api(&c,24u,119u,0x8893u,ibo,0u,0u);assert(ok(&c)==0u);
 api(&c,24u,120u,0x8893u,12u,0u,0x88e4u);assert(ok(&c)==0u);
 api(&c,24u,121u,0x8893u,2u,6u,idx);assert(ok(&c)==0u);
 api(&c,24u,27u,4u,3u,0x1403u,2u);assert(ok(&c)==0u);
 checked(&r,xyz,uv,reversed);

 /* Reject half-word misalignment before mutating GL pointer state. */
 const unsigned good=r.textured;
 api(&c,24u,78u,3u,0x1402u,5u,10u);stopped(&c,78u);
 assert(c.gl_vertex_stride==8u&&c.gl_vertex_pointer==10u);
 api(&c,24u,71u,2u,0x1402u,3u,70u);stopped(&c,71u);
 assert(c.gl_uv_stride==6u&&c.gl_uv_pointer==70u);
 api(&c,24u,78u,3u,0x1402u,8u,11u);stopped(&c,78u);
 api(&c,24u,71u,2u,0x1402u,6u,71u);stopped(&c,71u);
 assert(c.gl_vertex_pointer==10u&&c.gl_uv_pointer==70u);
 /* A valid binding may point beyond its VBO, but the DRAW must stop.
  * Never draw partial triangles or read host out-of-bounds. */
 api(&c,24u,78u,3u,0x1402u,8u,126u);assert(ok(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 assert(r.textured==good);
 api(&c,24u,78u,3u,0x1402u,8u,10u);assert(ok(&c)==0u);
 api(&c,24u,71u,2u,0x1402u,6u,127u);stopped(&c,71u);
 api(&c,24u,71u,2u,0x1402u,6u,126u);assert(ok(&c)==0u);
 api(&c,24u,26u,4u,0u,3u,0u);stopped(&c,26u);
 assert(r.textured==good);
 api(&c,24u,71u,2u,0x1402u,6u,70u);assert(ok(&c)==0u);
 r.reject=1;
 api(&c,24u,27u,4u,3u,0x1403u,2u);stopped(&c,27u);
 assert(r.textured==good);
 r.reject=0;
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: original Zeebx GL_SHORT signed XYZ/UV, interleaved and "
      "tight arrays, nonzero VBO/IBO byte offsets, safe GPU failure");
 return 0;
}
