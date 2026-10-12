/* ZeebxTeam/src/machine/gl.rs::le_texels + GL_LEGACY slots 57,74,76.
 * The original Crash GLES1 atlas update operates on ARM guest bytes.
 * No ROM or commercial assets used; callbacks are REAL guest-to-host ABI
 * contracts, not a demonstration of game playability.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 uint32_t bound,w,h,patch_x,patch_y,patch_w,patch_h;
 uint32_t uploads,patches;
 uint8_t pixels[96],patch[96];
 int fail;
} record;
static int start(void *u,uint32_t w,uint32_t h){
 (void)u;return w==640u&&h==480u?0:-1;
}
static int present(void *u){(void)u;return 0;}
static int clear(void *u,uint32_t mask){(void)u;(void)mask;return 0;}
static int color(void *u,float a,float b,float d,float e){
 (void)u;(void)a;(void)b;(void)d;(void)e;return 0;
}
static int viewport(void *u,int32_t x,int32_t y,uint32_t w,uint32_t h){
 (void)u;(void)x;(void)y;(void)w;(void)h;return 0;
}
static int bind(void *u,uint32_t name){
 record *r=(record*)u;
 if(r->fail)return -1;
 r->bound=name;
 return 0;
}
static int upload(void *u,uint32_t w,uint32_t h,
                  uint32_t f,uint32_t t,const uint8_t *p){
 record *r=(record*)u;
 if(r->fail||!r->bound||!p||w*h*2u>sizeof(r->pixels)||
    f!=0x1907u||t!=0x8363u)return -1;
 r->w=w;r->h=h;r->uploads++;
 memcpy(r->pixels,p,w*h*2u);
 return 0;
}
static int patch(void *u,uint32_t x,uint32_t y,uint32_t w,uint32_t h,
                 uint32_t f,uint32_t t,const uint8_t *p){
 record *r=(record*)u;
 if(r->fail||!r->bound||!r->uploads||!p||
    x+w>r->w||y+h>r->h||
    f!=0x1907u||t!=0x8363u||w*h*2u>sizeof(r->patch))return -1;
 r->patch_x=x;r->patch_y=y;r->patch_w=w;r->patch_h=h;
 memcpy(r->patch,p,w*h*2u);
 r->patches++;
 return 0;
}
static void put32(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));
}
static void call(cnk_zeebx_arm *c,unsigned iface,unsigned slot,
                 uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3){
 c->regs[0]=a0;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=a3;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+slot*4u;
}
static uint32_t ok(cnk_zeebx_arm *c){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,30u,&out,&handled));
 assert(handled==1u&&out.reason==CNK_ARM_RETURNED);
 return c->regs[0];
}
static void trap(cnk_zeebx_arm *c,unsigned slot){
 cnk_arm_outcome out={0};uint32_t handled=0;
 assert(!cnk_zeebx_arm_bootstrap(c,30u,&out,&handled));
 assert(out.reason==CNK_ARM_API&&out.api_interface==24u&&
        out.api_slot==slot&&handled==0u);
}
static void stack5(cnk_zeebx_arm *c,uint32_t a,uint32_t b,
                   uint32_t d,uint32_t e,uint32_t f){
 c->regs[13]=CNK_ARM_STACK_BASE+0x200u;
 put32(c->stack+0x200u,a);
 put32(c->stack+0x204u,b);
 put32(c->stack+0x208u,d);
 put32(c->stack+0x20cu,e);
 put32(c->stack+0x210u,f);
}
int main(void){
 uint8_t mod[128]={0};
 cnk_zeebx_arm c={0};
 record r={0};
 cnk_zeebx_gl_host host={0};
 host.make_current=start;host.present=present;
 host.clear=clear;host.clear_color=color;host.viewport=viewport;
 host.texture_bind=bind;host.texture_upload=upload;
 host.texture_sub_upload=patch;
 assert(!cnk_zeebx_arm_init(&c,mod,sizeof(mod)));
 assert(c.gl_unpack_alignment==4u);
 assert(!cnk_zeebx_arm_set_gl_host(&c,&host,&r));
 /* Genuine EGL legacy context is mandatory before GL guest calls. */
 call(&c,24u,57u,0x0cf5u,1u,0u,0u);trap(&c,57u);
 call(&c,23u,5u,1u,0u,0u,0u);assert(ok(&c)==1u);
 call(&c,23u,12u,1u,1u,0u,0u);
 const uint32_t surface=ok(&c);
 call(&c,23u,17u,1u,1u,0u,0u);
 const uint32_t context=ok(&c);
 call(&c,23u,19u,1u,surface,surface,context);
 assert(ok(&c)==1u);
 call(&c,24u,5u,0x0de1u,51u,0u,0u);
 assert(ok(&c)==0u&&r.bound==51u);

 const uint32_t full=CNK_ARM_HEAP_BASE+0x200u;
 for(unsigned i=0;i<32u;i++)c.heap[0x200u+i]=(uint8_t)(i+1u);
 stack5(&c,4u,0u,0x1907u,0x8363u,full);
 call(&c,24u,74u,0x0de1u,0u,0x1907u,4u);
 assert(ok(&c)==0u&&r.uploads==1u&&r.w==4u&&r.h==4u);
 assert(!memcmp(r.pixels,c.heap+0x200u,32u));

 /* Odd RGB565 width has a six-byte row padded to eight by GLES=4.
  * Zeebx must copy row 2 from +8, not +6, or textures skew. */
 const uint32_t src=CNK_ARM_HEAP_BASE+0x300u;
 for(unsigned i=0;i<6u;i++)c.heap[0x300u+i]=(uint8_t)(0x11u+i);
 c.heap[0x306u]=0xeau;c.heap[0x307u]=0xebu; /* padding */
 for(unsigned i=0;i<6u;i++)c.heap[0x308u+i]=(uint8_t)(0x51u+i);
 stack5(&c,3u,2u,0x1907u,0x8363u,src);
 call(&c,24u,76u,0x0de1u,0u,1u,1u);
 assert(ok(&c)==0u);
 assert(r.patches==1u&&c.gl_texture_sub_uploads==1u);
 assert(r.patch_x==1u&&r.patch_y==1u&&r.patch_w==3u&&r.patch_h==2u);
 for(unsigned i=0;i<6u;i++){
  assert(r.patch[i]==(uint8_t)(0x11u+i));
  assert(r.patch[6u+i]==(uint8_t)(0x51u+i));
 }
 /* PixelStorei(UNPACK,1) removes row padding, also for base TexImage. */
 call(&c,24u,57u,0x0cf5u,1u,0u,0u);
 assert(ok(&c)==0u&&c.gl_unpack_alignment==1u);
 for(unsigned i=0;i<12u;i++)c.heap[0x350u+i]=(uint8_t)(0x80u+i);
 stack5(&c,3u,2u,0x1907u,0x8363u,CNK_ARM_HEAP_BASE+0x350u);
 call(&c,24u,76u,0x0de1u,0u,0u,0u);
 assert(ok(&c)==0u&&r.patches==2u);
 for(unsigned i=0;i<12u;i++)
  assert(r.patch[i]==(uint8_t)(0x80u+i));
 /* Restoring default 4 also fixes initial TexImage uploads. */
 call(&c,24u,57u,0x0cf5u,4u,0u,0u);
 assert(ok(&c)==0u&&c.gl_unpack_alignment==4u);
 stack5(&c,2u,0u,0x1907u,0x8363u,src);
 call(&c,24u,74u,0x0de1u,0u,0x1907u,3u);
 assert(ok(&c)==0u&&r.uploads==2u);
 for(unsigned i=0;i<6u;i++){
  assert(r.pixels[i]==(uint8_t)(0x11u+i));
  assert(r.pixels[6u+i]==(uint8_t)(0x51u+i));
 }
 /* Out-of-range destination rectangles are rejected by a real host,
  * not "success" without updating the picture. */
 stack5(&c,2u,2u,0x1907u,0x8363u,src);
 call(&c,24u,76u,0x0de1u,0u,2u,0u);
 trap(&c,76u);assert(r.patches==2u);
 /* Invalid ARM pointers never reach the host. The last row needs 14
  * bytes from an aligned RGB565 3x2 source, not 12. */
 stack5(&c,3u,2u,0x1907u,0x8363u,
        CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-12u);
 call(&c,24u,76u,0x0de1u,0u,0u,0u);
 trap(&c,76u);assert(r.patches==2u);
 /* Bad enum/level/signed offset/size/unpack values and absent
  * callback must not be silently acknowledged. */
 call(&c,24u,57u,0x0cf5u,3u,0u,0u);trap(&c,57u);
 assert(c.gl_unpack_alignment==4u);
 call(&c,24u,57u,0xdeadbeefu,1u,0u,0u);trap(&c,57u);
 stack5(&c,3u,2u,0x1907u,0x8363u,src);
 call(&c,24u,76u,0x0de1u,1u,0u,0u);trap(&c,76u);
 call(&c,24u,76u,0x0de1u,0u,UINT32_MAX,0u);trap(&c,76u);
 stack5(&c,0u,2u,0x1907u,0x8363u,src);
 call(&c,24u,76u,0x0de1u,0u,0u,0u);trap(&c,76u);
 stack5(&c,3u,2u,0x1907u,0x1401u,src);
 call(&c,24u,76u,0x0de1u,0u,0u,0u);trap(&c,76u);
 stack5(&c,3u,2u,0x1907u,0x8363u,src);
 r.fail=1;
 call(&c,24u,76u,0x0de1u,0u,0u,0u);trap(&c,76u);
 r.fail=0;
 c.gl_host.texture_sub_upload=NULL;
 call(&c,24u,76u,0x0de1u,0u,0u,0u);trap(&c,76u);
 assert(r.patches==2u&&c.gl_texture_sub_uploads==2u);
 cnk_zeebx_arm_destroy(&c);
 puts("PASS: Zeebx GL_UNPACK_ALIGNMENT and TexImage2D/TexSubImage2D"
      " row-packing, bounds and true GPU callback errors");
 return 0;
}
