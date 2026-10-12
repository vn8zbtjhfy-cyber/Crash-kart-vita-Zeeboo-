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
 unsigned draws,draw_vertices;
 float triangle[9];
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
static int draw_triangles(void *p,const float *xyz,uint32_t count){
 recorded_gpu *g=(recorded_gpu*)p;
 if(g->reject)return -1;
 if(!xyz||count>CNK_ZEEBX_MAX_DRAW_VERTICES||count%3u)return -1;
 g->draws++;g->draw_vertices+=count;
 memcpy(g->triangle,xyz,sizeof(g->triangle));
 return 0;
}
static void guest_float(cnk_zeebx_arm *c,uint32_t address,float v){
 assert(address>=CNK_ARM_HEAP_BASE);
 uint32_t offset=address-CNK_ARM_HEAP_BASE;
 assert(offset+4u<=CNK_ARM_HEAP_BYTES);
 uint32_t bits=0;
 memcpy(&bits,&v,sizeof(bits));
 for(unsigned k=0;k<4u;k++)c->heap[offset+k]=(uint8_t)(bits>>(8u*k));
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
  .pop_matrix=pop_matrix,.translate=translate,.scale=scale,.rotate=rotate,
  .draw_triangles=draw_triangles
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
 /* Deliver REAL triangles from bounded guest ARM heap via glVertexPointer.
  * Vertex arrays are read at draw time, not decoded as host pointers. */
 const uint32_t base=CNK_ARM_HEAP_BASE+64u;
 const float triangle[9]={0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
 for(unsigned k=0;k<9u;k++)guest_float(&cpu,base+4u*k,triangle[k]);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u); /* nothing enabled or registered */
 api(&cpu,24u,78u,3u,0x1406u,0u,base);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u); /* vertex client array still disabled */
 api(&cpu,24u,29u,0x8074u,0u,0u,0u);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 assert(invoke(&cpu)==0u&&g.draws==1u&&g.draw_vertices==3u);
 assert(memcmp(g.triangle,triangle,sizeof(triangle))==0);
 assert(cpu.gl_draw_calls==1u&&cpu.gl_drawn_vertices==3u);
 /* Proper GL_FIXED vertex pointer remains unsupported, not misdecoded. */
 api(&cpu,24u,78u,3u,0x140cu,0u,base);
 trap(&cpu,24u,78u);
 api(&cpu,24u,78u,3u,0x1406u,11u,base);
 trap(&cpu,24u,78u);
 api(&cpu,24u,78u,3u,0x1406u,0u,0xffffffffu);
 trap(&cpu,24u,78u);
 /* Valid interleaved layout with 4-byte padding and starting offset. */
 const uint32_t padded=CNK_ARM_HEAP_BASE+256u;
 for(unsigned k=0;k<4u;k++)for(unsigned axis=0;axis<3u;axis++)
  guest_float(&cpu,padded+16u*k+4u*axis,k?triangle[(k-1u)*3u+axis]:99.f);
 api(&cpu,24u,78u,3u,0x1406u,16u,padded);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,1u,3u,0u);
 assert(invoke(&cpu)==0u&&g.draws==2u&&g.draw_vertices==6u);
 assert(memcmp(g.triangle,triangle,sizeof(triangle))==0);
 /* Guest buffer must have room for ALL requested vertices. */
 api(&cpu,24u,78u,3u,0x1406u,0u,CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-8u);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u);
 /* Nonfinite positions are always rejected before GPU sees them. */
 guest_float(&cpu,base+4u, __builtin_nanf(""));
 api(&cpu,24u,78u,3u,0x1406u,0u,base);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u);
 guest_float(&cpu,base+4u,0.f);
 api(&cpu,24u,26u,4u,0u,CNK_ZEEBX_MAX_DRAW_VERTICES+3u,0u);
 trap(&cpu,24u,26u);
 api(&cpu,24u,26u,4u,0xffffffffu,3u,0u);
 trap(&cpu,24u,26u);
 api(&cpu,24u,26u,5u,0u,3u,0u);
 trap(&cpu,24u,26u);
 api(&cpu,24u,25u,0x8074u,0u,0u,0u);
 assert(invoke(&cpu)==0u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u);
 assert(g.draws==2u && cpu.gl_draw_calls==2u);
 api(&cpu,24u,29u,0x8074u,0u,0u,0u);
 assert(invoke(&cpu)==0u);
 /* Real indexed triangle support: guest GL_UNSIGNED_SHORT and BYTE.
  * Indices [2,1,0] must reverse the same triangle, not reorder at GPU. */
 const uint32_t ibase=CNK_ARM_HEAP_BASE+1024u;
 uint32_t io=ibase-CNK_ARM_HEAP_BASE;
 cpu.heap[io]=2u;cpu.heap[io+1u]=0u;
 cpu.heap[io+2u]=1u;cpu.heap[io+3u]=0u;
 cpu.heap[io+4u]=0u;cpu.heap[io+5u]=0u;
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase);
 assert(invoke(&cpu)==0u && g.draws==3u && g.draw_vertices==9u);
 for(unsigned v=0;v<3u;v++)
  for(unsigned axis=0;axis<3u;axis++)
   assert(g.triangle[v*3u+axis]==triangle[(2u-v)*3u+axis]);
 assert(cpu.gl_draw_calls==3u&&cpu.gl_drawn_vertices==9u);
 cpu.heap[io+16u]=0u;cpu.heap[io+17u]=2u;cpu.heap[io+18u]=1u;
 api(&cpu,24u,27u,4u,3u,0x1401u,ibase+16u);
 assert(invoke(&cpu)==0u && g.draws==4u && g.draw_vertices==12u);
 for(unsigned axis=0;axis<3u;axis++){
  assert(g.triangle[axis]==triangle[axis]);
  assert(g.triangle[3u+axis]==triangle[6u+axis]);
  assert(g.triangle[6u+axis]==triangle[3u+axis]);
 }
 /* Typed, aligned, bounded indices must be checked before GPU callback. */
 api(&cpu,24u,27u,4u,3u,0x1405u,ibase); /* uint32 indices not GLES1 */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase+1u); /* misaligned */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,4u,3u,0x1401u,0u); /* null index pointer */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,4u,2u,0x1401u,ibase+16u); /* incomplete tri */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,5u,3u,0x1401u,ibase+16u); /* wrong primitive */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,4u,CNK_ZEEBX_MAX_DRAW_VERTICES+3u,
     0x1401u,ibase+16u); /* oversized request */
 trap(&cpu,24u,27u);
 api(&cpu,24u,27u,4u,3u,0x1403u,CNK_ARM_HEAP_BASE+
     CNK_ARM_HEAP_BYTES-4u); /* incomplete guest index buffer */
 trap(&cpu,24u,27u);
 cpu.heap[io]=0xffu;cpu.heap[io+1u]=0xffu; /* valid u16, invalid vertex */
 cpu.gl_vertex_pointer=CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES-48u;
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase);
 trap(&cpu,24u,27u);
 cpu.gl_vertex_pointer=base;
 cpu.heap[io]=2u;cpu.heap[io+1u]=0u;
 /* Invalid guest vertex (NaN) must prevent even an indexed draw. */
 guest_float(&cpu,base+4u,__builtin_nanf(""));
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase);
 trap(&cpu,24u,27u);
 guest_float(&cpu,base+4u,0.f);
 cnk_zeebx_gl_host saved=cpu.gl_host;
 cpu.gl_host.draw_triangles=NULL;
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase);
 trap(&cpu,24u,27u);
 cpu.gl_host=saved;
 assert(g.draws==4u&&cpu.gl_draw_calls==4u);
 api(&cpu,24u,7u,0x80000000u,0u,0u,0u);
 trap(&cpu,24u,7u);
 assert(g.clear==1u);
 g.reject=1;
 api(&cpu,24u,27u,4u,3u,0x1403u,ibase);
 trap(&cpu,24u,27u);
 assert(g.draws==4u&&cpu.gl_draw_calls==4u);
 api(&cpu,24u,26u,4u,0u,3u,0u);
 trap(&cpu,24u,26u);
 assert(g.draws==4u&&cpu.gl_draw_calls==4u);
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
 puts("PASS: Zeebx indexed and nonindexed triangles read bounded guest "
      "data; bad index types, buffers, values and GPU failures trap");
 return 0;
}
