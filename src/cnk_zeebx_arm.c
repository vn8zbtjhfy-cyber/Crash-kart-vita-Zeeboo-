/* Isolated execution bridge, adapted in C99 from:
 * ZeebxTeam/zeebx-emu/src/cpu/interpretador.rs
 * ZeebxTeam/zeebx-emu/src/loader/mod.rs
 * ZeebxTeam/zeebx-emu/src/brew/aee.rs
 * Copyright ZeebxTeam contributors, GPL-2.0-or-later.
 *
 * This is a deliberately limited subset of Zeebx's ARM1176 interpreter.
 * Real guest instructions run in virtual memory; unimplemented opcodes and
 * BREW calls STOP with explicit outcomes (never a fabricated working game).
 * No PS Vita ARM guest code is executed directly or mapped as native code.
 */
#include "cnk_zeebx_arm.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define N 0x80000000u
#define Z 0x40000000u
#define C 0x20000000u
#define V 0x10000000u
#define T 0x00000020u
#define MOD_MAX (16u*1024u*1024u)
#define OBJECT_BYTES 0x1000u
#define HELPER_BYTES 0x1000u
#define VTABLE_BYTES 0x10000u
static uint32_t word(const uint8_t *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|
        ((uint32_t)p[3]<<24);
}
static void putword(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8u*i));
}
void cnk_zeebx_arm_destroy(cnk_zeebx_arm *cpu){
 if(!cpu)return;
 free(cpu->module);free(cpu->stack);free(cpu->heap);free(cpu->objects);
 free(cpu->helpers);free(cpu->vtables);
 memset(cpu,0,sizeof(*cpu));
}
int cnk_zeebx_arm_init(cnk_zeebx_arm *cpu,const uint8_t *mod,size_t bytes){
 cnk_brew_mod_info info={0};
 if(!cpu)return -1;
 memset(cpu,0,sizeof(*cpu));
 if(!mod||!bytes||bytes>MOD_MAX||cnk_brew_mod_inspect(mod,bytes,&info))
  return -2;
 size_t size=(size_t)CNK_ARM_MOD_PREFIX+bytes+CNK_ARM_BSS_BYTES;
 cpu->module=(uint8_t*)calloc(1,size);
 cpu->stack=(uint8_t*)calloc(1,CNK_ARM_STACK_BYTES);
 cpu->heap=(uint8_t*)calloc(1,CNK_ARM_HEAP_BYTES);
 cpu->objects=(uint8_t*)calloc(1,OBJECT_BYTES);
 cpu->helpers=(uint8_t*)calloc(1,HELPER_BYTES);
 cpu->vtables=(uint8_t*)calloc(1,VTABLE_BYTES);
 if(!cpu->module||!cpu->stack||!cpu->heap||!cpu->objects||!cpu->helpers||
    !cpu->vtables){cnk_zeebx_arm_destroy(cpu);return -3;}
 cpu->module_size=size;
 memcpy(cpu->module+CNK_ARM_MOD_PREFIX,mod,bytes);
 /* Zeebx boot protocol: the two preceding words carry helper table
  * version and base address; Shell* at r0, helper* at r1, out IModule**
  * at r2, SP at top of one-megabyte stack. */
 putword(cpu->module+CNK_ARM_MOD_PREFIX-8,1u);
 putword(cpu->module+CNK_ARM_MOD_PREFIX-4,CNK_ARM_HELPERS_BASE);
 putword(cpu->objects,CNK_ARM_VTABLE_BASE);
 for(unsigned i=0;i<256;i++){
  putword(cpu->vtables+4u*i,CNK_ARM_API_BASE+i*4u); /* IShell */
  /* Zeebx's vtables are 256 slots each. The next applet classes
   * observed in genuine cnk2.mod are IDisplay (5) and IFileMgr (3).
   * Vtable slots lead to diagnostic API traps, not fake methods. */
  putword(cpu->vtables+3u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(3u<<12)+i*4u);
  putword(cpu->vtables+5u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(5u<<12)+i*4u);
  putword(cpu->vtables+7u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(7u<<12)+i*4u);
  putword(cpu->vtables+14u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(14u<<12)+i*4u);
  /* Crash requests these four BREW classes. Keep their Zeebx aee.rs
   * interface numbers intact; unknown methods STOP at their own slots
   * until ported, rather than pretending graphics or HID are working. */
  putword(cpu->vtables+8u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(8u<<12)+i*4u);   /* IHID */
  putword(cpu->vtables+9u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(9u<<12)+i*4u);   /* IHIDDevice */
  putword(cpu->vtables+10u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(10u<<12)+i*4u); /* ISignal */
  putword(cpu->vtables+11u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(11u<<12)+i*4u); /* ISignalCtl */
  putword(cpu->vtables+12u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(12u<<12)+i*4u); /* ISignalCBFactory */
  putword(cpu->vtables+23u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(23u<<12)+i*4u); /* legacy EGL */
  putword(cpu->vtables+24u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(24u<<12)+i*4u); /* legacy GL */
  putword(cpu->helpers+4u*i,CNK_ARM_API_BASE+(6u<<12)+i*4u);
 }
 cpu->regs[0]=CNK_ARM_OBJECT_BASE;
 cpu->regs[1]=CNK_ARM_HELPERS_BASE;
 cpu->regs[2]=CNK_ARM_OBJECT_BASE+8u;
 cpu->regs[13]=CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-16u;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_MOD_BASE+info.entry_offset;
 cpu->cpsr=0x10u; /* user mode ARM */
 for(unsigned i=0;i<CNK_ZEEBX_PAD_AXES;i++)cpu->pad.axes[i]=128u;
 cpu->heap_cursor=0u;
 cpu->egl_error=0x3000u; /* initial EGL_SUCCESS */
 cpu->shell_refs=1u;
 cpu->object_cursor=16u; /* Shell, Module**, IApplet** reserved */
 cpu->initialized=1;
 return 0;
}
static uint8_t *map(cnk_zeebx_arm *c,uint32_t addr,size_t n,int write){
 (void)write;
 uint64_t end=(uint64_t)addr+n;
 const uint32_t module_start=CNK_ARM_MOD_BASE-CNK_ARM_MOD_PREFIX;
 if(addr>=module_start &&
    (uint64_t)addr-module_start+n<=c->module_size)
  return c->module+(addr-module_start);
 if(addr>=CNK_ARM_STACK_BASE &&
    end<=(uint64_t)CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES)
  return c->stack+(addr-CNK_ARM_STACK_BASE);
 if(addr>=CNK_ARM_HEAP_BASE &&
    end<=(uint64_t)CNK_ARM_HEAP_BASE+CNK_ARM_HEAP_BYTES)
  return c->heap+(addr-CNK_ARM_HEAP_BASE);
 if(addr>=CNK_ARM_OBJECT_BASE &&
    end<=(uint64_t)CNK_ARM_OBJECT_BASE+OBJECT_BYTES)
  return c->objects+(addr-CNK_ARM_OBJECT_BASE);
 if(addr>=CNK_ARM_HELPERS_BASE &&
    end<=(uint64_t)CNK_ARM_HELPERS_BASE+HELPER_BYTES)
  return c->helpers+(addr-CNK_ARM_HELPERS_BASE);
 if(!write&&addr>=CNK_ARM_VTABLE_BASE &&
    end<=(uint64_t)CNK_ARM_VTABLE_BASE+VTABLE_BYTES)
  return c->vtables+(addr-CNK_ARM_VTABLE_BASE);
 return NULL;
}
static int load(cnk_zeebx_arm *c,uint32_t addr,unsigned bytes,uint32_t *v){
 if((bytes==4&&(addr&3u))||(bytes==2&&(addr&1u)))return -1;
 uint8_t *p=map(c,addr,bytes,0);
 if(!p)return -1;
 *v=bytes==1?p[0]:bytes==2?(uint32_t)p[0]|((uint32_t)p[1]<<8):word(p);
 return 0;
}
static int store(cnk_zeebx_arm *c,uint32_t addr,unsigned bytes,uint32_t v){
 if((bytes==4&&(addr&3u))||(bytes==2&&(addr&1u)))return -1;
 uint8_t *p=map(c,addr,bytes,1);
 if(!p)return -1;
 for(unsigned i=0;i<bytes;i++)p[i]=(uint8_t)(v>>(8u*i));
 return 0;
}
static int cond(uint32_t psr,unsigned q){
 int n=(psr&N)!=0,z=(psr&Z)!=0,c=(psr&C)!=0,v=(psr&V)!=0;
 switch(q){
 case 0:return z;case 1:return !z;
 case 2:return c;case 3:return !c;
 case 4:return n;case 5:return !n;
 case 6:return v;case 7:return !v;
 case 8:return c&&!z;case 9:return !c||z;
 case 10:return n==v;case 11:return n!=v;
 case 12:return !z&&(n==v);case 13:return z||(n!=v);
 case 14:return 1;default:return 0;
 }
}
static uint32_t regval(const cnk_zeebx_arm *c,unsigned r,uint32_t pc){
 return r==15?pc+8u:c->regs[r];
}
static uint32_t ror32(uint32_t x,unsigned by){
 by&=31u;
 return by?((x>>by)|(x<<(32u-by))):x;
}
/* Adapt Zeebx ARM barrel shifter: RRX, LSR/ASR #0=32,
 * register shifts with zero meaning "unchanged". */
static uint32_t shift(uint32_t x,unsigned type,unsigned count,
                      int oldc,int immediate,int *carry){
 *carry=oldc;
 if(!immediate&&!count)return x;
 if(type==0){
  if(!count)return x;
  if(count<32u){*carry=(int)((x>>(32u-count))&1u);return x<<count;}
  *carry=count==32u?(int)(x&1u):0;return 0;
 }
 if(type==1){
  unsigned n=immediate&&!count?32u:count;
  if(n<32u){*carry=(int)((x>>(n-1u))&1u);return x>>n;}
  *carry=n==32u?(int)(x>>31):0;return 0;
 }
 if(type==2){
  unsigned n=immediate&&!count?32u:count;
  if(n<32u){*carry=(int)((x>>(n-1u))&1u);return (uint32_t)((int32_t)x>>n);}
  *carry=(int)(x>>31);
  return (x&N)?UINT32_MAX:0;
 }
 if(immediate&&!count){
  *carry=(int)(x&1u);return (x>>1)|(oldc?N:0u);
 }
 unsigned n=count&31u;
 if(!n){*carry=(int)(x>>31);return x;}
 x=ror32(x,n);*carry=(int)(x>>31);return x;
}
static int operand(cnk_zeebx_arm *c,uint32_t op,uint32_t pc,
                   uint32_t *value,int *carry){
 unsigned rm=op&15u,type=(op>>5)&3u;
 int shift_reg=(op&(1u<<4))!=0u;
 if(shift_reg&&(op&(1u<<7)))return -1;
 uint32_t r=regval(c,rm,pc);
 unsigned count=shift_reg?(regval(c,(op>>8)&15u,pc)&255u):
                          ((op>>7)&31u);
 *value=shift(r,type,count,(c->cpsr&C)!=0,!shift_reg,carry);
 return 0;
}
static void flag_nz(cnk_zeebx_arm *c,uint32_t val){
 c->cpsr=(c->cpsr&~(N|Z))|(val&N)|(val?0u:Z);
}
static void flag_cv(cnk_zeebx_arm *c,int carry,int overflow){
 c->cpsr=(c->cpsr&~(C|V))|(carry?C:0u)|(overflow?V:0u);
}
static int addcalc(uint32_t a,uint32_t b,uint32_t carry,
                   uint32_t *r,int *cout,int *ov){
 uint64_t wide=(uint64_t)a+b+carry;
 *r=(uint32_t)wide;*cout=(int)(wide>>32);
 *ov=(((a^*r)&(b^*r)&N)!=0u);
 return 0;
}
static int subcalc(uint32_t a,uint32_t b,uint32_t borrow,
                   uint32_t *r,int *cout,int *ov){
 uint64_t total=(uint64_t)b+borrow;
 *r=(uint32_t)((uint64_t)a-total);
 *cout=(uint64_t)a>=total;
 *ov=(((a^b)&(a^*r)&N)!=0u);
 return 0;
}
static int bx(cnk_zeebx_arm *c,uint32_t target){
 if(target&1u){c->cpsr|=T;c->regs[15]=target&~1u;}
 else {c->cpsr&=~T;c->regs[15]=target&~3u;}
 return 0;
}
static cnk_arm_reason step(cnk_zeebx_arm *c,uint32_t *bad){
 uint32_t pc=c->regs[15],op=0; *bad=0;
 if(pc==CNK_ARM_RETURN_MAGIC)return CNK_ARM_RETURNED;
 if(pc>=CNK_ARM_API_BASE && pc<CNK_ARM_API_BASE+0x01000000u)
  return CNK_ARM_API;
 if(c->cpsr&T){
  *bad=pc;return CNK_ARM_UNSUPPORTED;
 }
 /* Zeebx GuestMemory marks only the module executable. In the C bridge
  * the heap, stack, vtables and API scratch buffers are DATA, never
  * host/guest executable pages. Report an invalid guest control flow. */
 uint64_t exec_end=(uint64_t)CNK_ARM_MOD_BASE+
                        c->module_size-CNK_ARM_MOD_PREFIX;
 if((pc&3u)||pc<CNK_ARM_MOD_BASE||(uint64_t)pc+4u>exec_end){
  *bad=pc;return CNK_ARM_FAULT;
 }
 if(load(c,pc,4,&op)){*bad=pc;return CNK_ARM_FAULT;}
 *bad=op;
 unsigned condition=op>>28;
 if(condition==15u){
  /* BLX immediate (Zeebx interpreter, ARMv5+). */
  if(((op>>25)&7u)==5u){
   int32_t words=(int32_t)((op&0xffffffu)<<8)>>8;
   uint32_t target=pc+8u+(uint32_t)(words*4)+((op>>24)&1u)*2u;
   c->regs[14]=pc+4u;
   c->cpsr|=T;
   c->regs[15]=target&~1u;
   return CNK_ARM_BUDGET;
  }
  return CNK_ARM_UNSUPPORTED;
 }
 if(!cond(c->cpsr,condition)){
  c->regs[15]=pc+4u;return CNK_ARM_BUDGET;
 }
 unsigned family=(op>>25)&7u;
 if(family==5u){ /* B / BL */
  int32_t signed_words=(int32_t)((op&0xffffffu)<<8)>>8;
  if(op&(1u<<24))c->regs[14]=pc+4u;
  c->regs[15]=pc+8u+(uint32_t)(signed_words*4);
  return CNK_ARM_BUDGET;
 }
 if((op&0x0ffffff0u)==0x012fff10u){ /* BX */
  bx(c,regval(c,op&15u,pc));return CNK_ARM_BUDGET;
 }
 if((op&0x0ffffff0u)==0x012fff30u){ /* BLX register */
  c->regs[14]=pc+4u;
  bx(c,regval(c,op&15u,pc));return CNK_ARM_BUDGET;
 }
 if(family==4u){ /* LDM/STM, Zeebx ARM register-order rules */
  unsigned rn=(op>>16)&15u;
  uint32_t mask=op&0xffffu,base=regval(c,rn,pc);
  int pre=(op&(1u<<24))!=0,up=(op&(1u<<23))!=0;
  int wb=(op&(1u<<21))!=0,reading=(op&(1u<<20))!=0;
  unsigned count=0;for(unsigned i=0;i<16;i++)if(mask&(1u<<i))count++;
  if(!count)return CNK_ARM_UNSUPPORTED;
  uint32_t bytes=count*4u;
  uint32_t addr=up?(base+(pre?4u:0u)):
                   (base-bytes+(pre?0u:4u));
  uint32_t next=up?base+bytes:base-bytes;
  uint32_t values[16]={0};
  for(unsigned i=0,k=0;i<16;i++)if(mask&(1u<<i)){
   if(reading){
    if(load(c,addr,4,&values[i])){*bad=addr;return CNK_ARM_FAULT;}
   }else {
    values[i]=regval(c,i,pc);
    if(store(c,addr,4,values[i])){*bad=addr;return CNK_ARM_FAULT;}
   }
   addr+=4u;k++;
  }
  int branch=0;
  if(reading)for(unsigned i=0;i<16;i++)if(mask&(1u<<i)){
   if(i==15){bx(c,values[i]);branch=1;}
   else c->regs[i]=values[i];
  }
  if(wb&&rn!=15u&&(!reading||!(mask&(1u<<rn))))c->regs[rn]=next;
  if(!branch)c->regs[15]=pc+4u;
  return CNK_ARM_BUDGET;
 }
 if(family==2u||family==3u){ /* LDR/STR; Zeebx pre/post-index */
  unsigned rn=(op>>16)&15u,rd=(op>>12)&15u;
  int regoffset=(op&(1u<<25))!=0,pre=(op&(1u<<24))!=0;
  int up=(op&(1u<<23))!=0,byte=(op&(1u<<22))!=0;
  int writeback=(op&(1u<<21))!=0,reading=(op&(1u<<20))!=0;
  uint32_t offset=op&0xfffu;
  if(regoffset){
   if(op&(1u<<4))return CNK_ARM_UNSUPPORTED;
   int carry=0;
   if(operand(c,op,pc,&offset,&carry))return CNK_ARM_UNSUPPORTED;
  }
  uint32_t base=regval(c,rn,pc);
  uint32_t changed=up?base+offset:base-offset;
  uint32_t addr=pre?changed:base,val=0;
  if(reading){
   if(load(c,addr,byte?1u:4u,&val)){*bad=addr;return CNK_ARM_FAULT;}
  }else{
   val=regval(c,rd,pc);
   if(store(c,addr,byte?1u:4u,val)){*bad=addr;return CNK_ARM_FAULT;}
  }
  if((writeback||!pre)&&rn!=15u)c->regs[rn]=changed;
  if(reading&&rd==15u)bx(c,val);
  else{
   if(reading)c->regs[rd]=val;
   c->regs[15]=pc+4u;
  }
  return CNK_ARM_BUDGET;
 }
 if(family==0u && (op&0x0e000090u)==0x00000090u &&
    ((op>>5)&3u)!=0u){ /* LDRH / LDRSB / LDRSH / STRH */
  int pre=(op&(1u<<24))!=0,up=(op&(1u<<23))!=0;
  int imm=(op&(1u<<22))!=0,writeback=(op&(1u<<21))!=0;
  int reading=(op&(1u<<20))!=0;
  unsigned rn=(op>>16)&15u,rd=(op>>12)&15u,sh=(op>>5)&3u;
  uint32_t offset=imm?(((op>>4)&0xf0u)|(op&0xfu)):
                       regval(c,op&15u,pc);
  uint32_t base=regval(c,rn,pc),changed=up?base+offset:base-offset;
  uint32_t addr=pre?changed:base,val=0;
  if(reading){
   if(load(c,addr,sh==2u?1u:2u,&val)){*bad=addr;return CNK_ARM_FAULT;}
   if(sh==2u)val=(uint32_t)(int32_t)(int8_t)val;
   if(sh==3u)val=(uint32_t)(int32_t)(int16_t)val;
  }else{
   if(sh!=1u)return CNK_ARM_UNSUPPORTED;
   val=regval(c,rd,pc);
   if(store(c,addr,2,val)){*bad=addr;return CNK_ARM_FAULT;}
  }
  if((writeback||!pre)&&rn!=15u)c->regs[rn]=changed;
  if(reading&&rd==15u)bx(c,val);
  else{
   if(reading)c->regs[rd]=val;
   c->regs[15]=pc+4u;
  }
  return CNK_ARM_BUDGET;
 }
 if(family==0u&&(op&0x0fc000f0u)==0x00000090u){ /* MUL MLA */
  unsigned rd=(op>>16)&15u,rn=(op>>12)&15u;
  uint32_t rm=regval(c,op&15u,pc),rs=regval(c,(op>>8)&15u,pc);
  uint32_t v=rm*rs;
  if(op&(1u<<21))v+=regval(c,rn,pc);
  if(rd==15u)return CNK_ARM_UNSUPPORTED;
  c->regs[rd]=v;
  if(op&(1u<<20))flag_nz(c,v);
  c->regs[15]=pc+4u;
  return CNK_ARM_BUDGET;
 }
 /* ARMv5 PSR transfers are used by BREW startup code. In user mode
  * only the CPSR flag byte can be written; banked SPSR and privileged
  * control fields remain explicitly unsupported. */
 if((op&0x0fbf0fffu)==0x010f0000u){ /* MRS Rd,CPSR */
  unsigned rd=(op>>12)&15u;
  if((op&(1u<<22))||rd==15u)return CNK_ARM_UNSUPPORTED;
  c->regs[rd]=c->cpsr;
  c->regs[15]=pc+4u;
  return CNK_ARM_BUDGET;
 }
 if((op&0x0fb0fff0u)==0x0120f000u ||
    (op&0x0fb0f000u)==0x0320f000u){ /* MSR CPSR_f,reg/imm */
  unsigned fields=(op>>16)&15u;
  int immediate=(op&(1u<<25))!=0;
  if((op&(1u<<22))||fields!=8u||
     (!immediate&&(op&15u)==15u))return CNK_ARM_UNSUPPORTED;
  uint32_t value=immediate?
   ror32(op&255u,((op>>8)&15u)*2u):regval(c,op&15u,pc);
  c->cpsr=(c->cpsr&~0xf8000000u)|(value&0xf8000000u);
  c->regs[15]=pc+4u;
  return CNK_ARM_BUDGET;
 }
 if(family<=1u){ /* ARM data processing, Zeebx ALU and shifter */
  int immediate=(op&(1u<<25))!=0;
  unsigned opcode=(op>>21)&15u, rn=(op>>16)&15u,rd=(op>>12)&15u;
  int update=(op&(1u<<20))!=0;
  if(opcode>=8u&&opcode<=11u&&!update)return CNK_ARM_UNSUPPORTED;
  uint32_t b=0;
  int shcarry=(c->cpsr&C)!=0;
  if(immediate){
   unsigned rot=((op>>8)&15u)*2u;
   b=ror32(op&255u,rot);
   if(rot)shcarry=(int)(b>>31);
  }else if(operand(c,op,pc,&b,&shcarry))return CNK_ARM_UNSUPPORTED;
  uint32_t a=regval(c,rn,pc),value=0;
  int carry=0,ov=0,write=1,logical=0;
  int oldc=(c->cpsr&C)!=0;
  switch(opcode){
   case 0:value=a&b;logical=1;break;
   case 1:value=a^b;logical=1;break;
   case 2:subcalc(a,b,0,&value,&carry,&ov);break;
   case 3:subcalc(b,a,0,&value,&carry,&ov);break;
   case 4:addcalc(a,b,0,&value,&carry,&ov);break;
   case 5:addcalc(a,b,(uint32_t)oldc,&value,&carry,&ov);break;
   case 6:subcalc(a,b,(uint32_t)!oldc,&value,&carry,&ov);break;
   case 7:subcalc(b,a,(uint32_t)!oldc,&value,&carry,&ov);break;
   case 8:value=a&b;logical=1;write=0;break;
   case 9:value=a^b;logical=1;write=0;break;
   case 10:subcalc(a,b,0,&value,&carry,&ov);write=0;break;
   case 11:addcalc(a,b,0,&value,&carry,&ov);write=0;break;
   case 12:value=a|b;logical=1;break;
   case 13:value=b;logical=1;break;
   case 14:value=a&~b;logical=1;break;
   default:value=~b;logical=1;break;
  }
  if(update&&rd==15u&&write)return CNK_ARM_UNSUPPORTED;
  if(update){
   flag_nz(c,value);
   if(logical){
    if(shcarry)c->cpsr|=C;else c->cpsr&=~C;
   }else flag_cv(c,carry,ov);
  }
  if(write&&rd==15u)c->regs[15]=value&~3u;
  else{
   if(write)c->regs[rd]=value;
   c->regs[15]=pc+4u;
  }
  return CNK_ARM_BUDGET;
 }
 return CNK_ARM_UNSUPPORTED;
}
int cnk_zeebx_arm_run(cnk_zeebx_arm *c,uint32_t budget,cnk_arm_outcome *out){
 if(!c||!out||!c->initialized||!budget||budget>10000000u)return -1;
 cnk_arm_outcome result={0};
 for(uint32_t i=0;i<budget;i++){
  uint32_t bad=0,pc=c->regs[15];
  cnk_arm_reason reason=step(c,&bad);
  if(reason!=CNK_ARM_BUDGET){
   result.reason=reason;result.pc=pc;
   result.address=(reason==CNK_ARM_FAULT)?bad:pc;
   result.opcode=(reason==CNK_ARM_UNSUPPORTED)?bad:0u;
   result.executed=c->executed;
   if(reason==CNK_ARM_API){
    result.api_interface=(pc-CNK_ARM_API_BASE)>>12;
    result.api_slot=((pc-CNK_ARM_API_BASE)&0xfffu)/4u;
   }
   *out=result;return 0;
  }
  c->executed++;
 }
 result.reason=CNK_ARM_BUDGET;
 result.pc=c->regs[15];result.executed=c->executed;
 *out=result;return 0;
}
int cnk_zeebx_arm_return_from_api(cnk_zeebx_arm *c,uint32_t result){
 if(!c||!c->initialized||c->regs[15]<CNK_ARM_API_BASE||
    c->regs[15]>=CNK_ARM_API_BASE+0x01000000u)return -1;
 c->regs[0]=result;
 return bx(c,c->regs[14]);
}

int cnk_zeebx_arm_read_u32(cnk_zeebx_arm *c,uint32_t addr,uint32_t *out){
 if(!c||!c->initialized||!out)return -1;
 return load(c,addr,4,out);
}
/* Only these exact Zeebx slots have experimentally verified startup
 * semantics for this game. All other BREW services remain unresolved. */
int cnk_zeebx_arm_set_gl_host(cnk_zeebx_arm *c,
                              const cnk_zeebx_gl_host *host,void *user){
 if(!c||!c->initialized)return -1;
 if(!host){
  if(c->egl_current_context||c->egl_current_surface)return -2;
  memset(&c->gl_host,0,sizeof(c->gl_host));
  c->gl_host_user=NULL;
  return 0;
 }
 if(!host->make_current||!host->clear_color||!host->clear||
    !host->viewport||!host->present)return -3;
 if(c->egl_current_context||c->egl_current_surface)return -2;
 c->gl_host=*host;c->gl_host_user=user;
 return 0;
}
int cnk_zeebx_arm_set_virtual_time(cnk_zeebx_arm *c,uint32_t ms){
 if(!c||!c->initialized||ms<c->virtual_time_ms)return -1;
 c->virtual_time_ms=ms;
 return 0;
}
/* Source-backed Zeebx input/mod.rs identifiers, original Zeebo HID SDK.
 * Preserve the exact 18 entries/order including duplicated unused UID.
 * Values are not guessed runtime events: only host-provided pad snapshots.
 */
static const uint32_t zeebx_button_uids[CNK_ZEEBX_PAD_BUTTONS]={
 0x0106c40bu,0x0106c40au,0x0106c40cu,0x0106c40du,
 0x0106c408u,0x0106c407u,0x0106c406u,0x0106c409u,
 0x0106c405u,0x0106c403u,0x0106c404u,0x0106c402u,
 0x0106c3feu,0x0106c3ffu,0x0106c400u,0x0106c401u,
 0x0106c4d0u,0x0106c408u
};
static const uint32_t zeebx_axis_uids[CNK_ZEEBX_PAD_AXES]={
 0x0106c4d0u,0x0106c4d1u,0x0106c4ceu,0x0106c4cfu
};
static const unsigned zeebx_axis_slots[CNK_ZEEBX_PAD_AXES]={1,2,3,6};

static cnk_zeebx_signal *signal_for(cnk_zeebx_arm *c,uint32_t obj){
 for(unsigned i=0;i<c->signal_count;i++){
  cnk_zeebx_signal *s=&c->signals[i];
  if(obj==s->signal_obj&&s->signal_refs)return s;
  if(obj==s->control_obj&&s->control_refs)return s;
 }
 return NULL;
}
static int queue_signal(cnk_zeebx_arm *c,uint32_t signal_obj){
 if(!signal_obj)return 0;
 cnk_zeebx_signal *s=signal_for(c,signal_obj);
 if(!s||signal_obj!=s->signal_obj)return -3;
 if(!s->attached||!s->enabled||!s->callback)return 0;
 if(c->signal_pending_count>=CNK_ZEEBX_PENDING_SIGNALS)return -2;
 uint32_t tail=(c->signal_pending_head+c->signal_pending_count)%
               CNK_ZEEBX_PENDING_SIGNALS;
 c->signal_pending[tail].function=s->callback;
 c->signal_pending[tail].context=s->context;
 c->signal_pending_count++;
 return 0;
}

int cnk_zeebx_arm_set_pad(cnk_zeebx_arm *c,const cnk_zeebx_pad *pad){
 if(!c||!c->initialized||!pad||
    (pad->buttons & ~((1u<<CNK_ZEEBX_PAD_BUTTONS)-1u)))return -1;
 /* Preserve the source's per-button press AND release events in ascending
  * HID index order. A still-held button does not repeat each frame.
  * Overflow evicts the oldest event and marks the next read as dropped. */
 uint32_t changed=c->pad.buttons^pad->buttons;
 int axes_changed=memcmp(c->pad.axes,pad->axes,CNK_ZEEBX_PAD_AXES)!=0;
 /* A pending callback queue cannot silently lose signals. Reject a host
  * snapshot BEFORE writing its button events if notifications would not
  * fit. The input's previous state is then still intact. */
 unsigned notifications=0;
 cnk_zeebx_signal *button=signal_for(c,c->hid_button_signal);
 cnk_zeebx_signal *position=signal_for(c,c->hid_position_signal);
 if(changed&&button&&button->enabled&&button->attached&&button->callback)
  notifications++;
 if(axes_changed&&position&&position->enabled&&position->attached&&
    position->callback)notifications++;
 if(notifications>CNK_ZEEBX_PENDING_SIGNALS-c->signal_pending_count)
  return -2;
 for(unsigned i=0;i<CNK_ZEEBX_PAD_BUTTONS;i++)if(changed&(1u<<i)){
  if(c->hid_queue_count==CNK_ZEEBX_HID_QUEUE){
   c->hid_queue_head=(c->hid_queue_head+1u)%CNK_ZEEBX_HID_QUEUE;
   c->hid_queue_count--;
   c->hid_events_dropped=1u;
  }
  uint32_t tail=(c->hid_queue_head+c->hid_queue_count)%
                 CNK_ZEEBX_HID_QUEUE;
  c->hid_queue[tail].index=i;
  c->hid_queue[tail].pressed=(pad->buttons>>i)&1u;
  c->hid_queue[tail].at_ms=c->virtual_time_ms;
  c->hid_queue_count++;
 }
 if(changed&&c->hid_button_signal &&
    queue_signal(c,c->hid_button_signal))return -2;
 if(axes_changed&&c->hid_position_signal &&
    queue_signal(c,c->hid_position_signal))return -2;
 c->pad=*pad;
 return 0;
}
/* IHIDDevice slots from Zeebx brew/aee_slots.rs, not a guessed ABI.
 * The method's object must be a real guest IHIDDevice. All guest output
 * spans are checked BEFORE writing. Bad pointers remain explicit stops.
 */
static int zeebx_device_input(cnk_zeebx_arm *c,unsigned slot){
 uint32_t vtbl=0;
 if(load(c,c->regs[0],4u,&vtbl)||
    vtbl!=CNK_ARM_VTABLE_BASE+9u*CNK_ARM_VTABLE_STRIDE)return -3;
 const uint32_t output=c->regs[1];
 if(slot==9u){ /* GetNextButtonEvent(info*, timestamp*, dropped*) */
  const uint32_t info=c->regs[1],timestamp=c->regs[2],dropped=c->regs[3];
  /* A rejected call must not consume the event, partially write data or
   * clear the overflow report. Validate all output spans up front. */
  if((info&&((info&3u)||!map(c,info,20u,1)))||
     (timestamp&&((timestamp&3u)||!map(c,timestamp,4u,1)))||
     (dropped&&((dropped&3u)||!map(c,dropped,4u,1))))return -3;
  const int has=c->hid_queue_count!=0u;
  cnk_zeebx_hid_event ev={0};
  if(has)ev=c->hid_queue[c->hid_queue_head];
  if(info){
   uint8_t *out=map(c,info,20u,1);
   putword(out+0u,has?ev.index:0u);
   putword(out+4u,has?ev.pressed:0u);
   putword(out+8u,has?zeebx_button_uids[ev.index]:0u);
   putword(out+12u,0u);
   putword(out+16u,has?1u:0u);
  }
  if(timestamp && store(c,timestamp,4u,has?ev.at_ms:c->virtual_time_ms))
   return -3;
  if(dropped && store(c,dropped,4u,c->hid_events_dropped))
   return -3;
  if(has){
   c->hid_queue_head=(c->hid_queue_head+1u)%CNK_ZEEBX_HID_QUEUE;
   c->hid_queue_count--;
  }
  c->hid_events_dropped=0u;
  return cnk_zeebx_arm_return_from_api(c,has?0u:1u);
 }
 if(slot==7u){ /* GetNumberOfButtons(uint32*) */
  if(output && store(c,output,4u,CNK_ZEEBX_PAD_BUTTONS))return -3;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(slot==6u){ /* GetButtonInfo(id/UID, AEEHIDButtonInfo*) */
  const uint32_t requested=c->regs[1],out=c->regs[2];
  unsigned idx=CNK_ZEEBX_PAD_BUTTONS;
  /* The Zeebo SDK accepts both index and the actual UID. */
  for(unsigned i=0;i<CNK_ZEEBX_PAD_BUTTONS;i++)
   if(requested==zeebx_button_uids[i]){idx=i;break;}
  if(idx==CNK_ZEEBX_PAD_BUTTONS&&requested<CNK_ZEEBX_PAD_BUTTONS)
   idx=(unsigned)requested;
  if(idx==CNK_ZEEBX_PAD_BUTTONS)return -4;
  if(out){
   uint8_t *p=map(c,out,20u,1);
   if(!p || (out&3u))return -3;
   putword(p+0u,idx);
   putword(p+4u,(c->pad.buttons>>idx)&1u);
   putword(p+8u,zeebx_button_uids[idx]);
   putword(p+12u,0u);
   putword(p+16u,1u);
  }
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(slot>=10u&&slot<=13u){
  /* 25 words: boolean bRelativeAxes and 24 axis slots. */
  if(output){
   uint8_t *dst=map(c,output,25u*4u,1);
   if(!dst || (output&3u))return -3;
   /* GetPositionState: unused axes must be centered at 128, not 0.
    * GetAxesInfo: unused axes have UID=0, meaning "no axis".
    * Min/Max: all 24 possible axes need a proper range.
    */
   for(unsigned i=0;i<25u;i++){
    uint32_t value=0;
    if(i && (slot==10u||slot==12u))value=slot==10u?128u:255u;
    putword(dst+4u*i,value);
   }
   for(unsigned i=0;i<CNK_ZEEBX_PAD_AXES;i++){
    uint32_t value=slot==10u?(uint32_t)c->pad.axes[i]:
                   slot==13u?zeebx_axis_uids[i]:
                   slot==12u?255u:0u;
    putword(dst+4u*zeebx_axis_slots[i],value);
   }
  }
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 return -4;
}

/* From Zeebx machine/signal.rs and BREW aee_slots.rs. CreateSignal is
 * five-argument ARM AAPCS: (factory,callback,context,ISignal**,
 * ISignalCtl**) with the fifth argument on the guest stack.
 * Callbacks are QUEUED and must be delivered at a safe guest boundary.
 */
static int zeebx_signal_api(cnk_zeebx_arm *c,unsigned iface,unsigned slot){
 const uint32_t self=c->regs[0];
 uint32_t vtbl=0;
 if(load(c,self,4u,&vtbl)||
    vtbl!=CNK_ARM_VTABLE_BASE+iface*CNK_ARM_VTABLE_STRIDE)
  return -3;
 if(iface==12u&&slot==3u){
  const uint32_t out_signal=c->regs[3];
  uint32_t out_control=0;
  if(load(c,c->regs[13],4u,&out_control))return -3;
  if((out_signal&&((out_signal&3u)||!map(c,out_signal,4u,1)))||
     (out_control&&((out_control&3u)||!map(c,out_control,4u,1))))
   return -3;
  if(c->signal_count>=CNK_ZEEBX_MAX_SIGNALS||
     c->object_cursor>OBJECT_BYTES-128u)return -2;
  const uint32_t s=CNK_ARM_OBJECT_BASE+c->object_cursor,ctl=s+64u;
  if(store(c,s,4u,CNK_ARM_VTABLE_BASE+10u*CNK_ARM_VTABLE_STRIDE)||
     store(c,ctl,4u,CNK_ARM_VTABLE_BASE+11u*CNK_ARM_VTABLE_STRIDE))
   return -3;
  if(out_signal&&store(c,out_signal,4u,s))return -3;
  if(out_control&&store(c,out_control,4u,ctl))return -3;
  cnk_zeebx_signal *binding=&c->signals[c->signal_count++];
  binding->signal_obj=s;binding->control_obj=ctl;
  binding->callback=c->regs[1];binding->context=c->regs[2];
  binding->signal_refs=1u;binding->control_refs=1u;
  binding->attached=1u;binding->enabled=1u;
  c->object_cursor+=128u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface!=10u&&iface!=11u)return -4;
 cnk_zeebx_signal *s=signal_for(c,self);
 if(!s)return -3;
 uint32_t *refs=iface==10u?&s->signal_refs:&s->control_refs;
 if(slot==0u){ /* AddRef */
  if(*refs==UINT32_MAX)return -2;
  (*refs)++;
  return cnk_zeebx_arm_return_from_api(c,*refs);
 }
 if(slot==1u){ /* Release */
  if(!*refs)return -3;
  (*refs)--;
  if(iface==10u&&!*refs){
   s->attached=0u;
   if(c->hid_button_signal==self)c->hid_button_signal=0u;
   if(c->hid_position_signal==self)c->hid_position_signal=0u;
  }
  return cnk_zeebx_arm_return_from_api(c,*refs);
 }
 if(slot==2u){ /* QueryInterface: Zeebx reuses the same object */
  const uint32_t out=c->regs[2];
  if(out&&((out&3u)||!map(c,out,4u,1)))return -3;
  if(out&&store(c,out,4u,self))return -3;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(slot==3u){ /* ISignal/ISignalCtl::Set */
  const int rc=queue_signal(c,s->signal_obj);
  if(rc)return rc;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface==11u&&slot==4u){ /* ISignalCtl::Detach */
  s->attached=0u;
  if(c->hid_button_signal==s->signal_obj)c->hid_button_signal=0u;
  if(c->hid_position_signal==s->signal_obj)c->hid_position_signal=0u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface==11u&&slot==5u){ /* ISignalCtl::Enable */
  s->enabled=1u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 return -4;
}
static int zeebx_register_pad_signal(cnk_zeebx_arm *c,unsigned slot){
 const uint32_t device=c->regs[0],signal=c->regs[1];
 uint32_t vtbl=0;
 if(load(c,device,4u,&vtbl)||
    vtbl!=CNK_ARM_VTABLE_BASE+9u*CNK_ARM_VTABLE_STRIDE)
  return -3;
 if(signal){
  cnk_zeebx_signal *s=signal_for(c,signal);
  if(!s || s->signal_obj!=signal)return -3;
 }
 /* Zeebx sends an initial position callback even when the stick has not
  * moved: games need this to calibrate the axis UID/range at startup. */
 if(slot==14u&&signal){
  const int rc=queue_signal(c,signal);
  if(rc)return rc;
 }
 if(slot==8u)c->hid_button_signal=signal;
 else c->hid_position_signal=signal;
 return cnk_zeebx_arm_return_from_api(c,0u);
}

/* Original IEGL from Zeebx brew/aee_slots.rs and src/machine/egl.rs.
 * This OLD AEEGL.h vtable does NOT pass the object as r0, unlike IEGL11.
 * The guest receives only a configuration descriptor here. In particular,
 * eglCreateContext/eglMakeCurrent/eglSwapBuffers intentionally remain
 * UNIMPLEMENTED until an actual VitaGL rendering bridge is connected.
 * Returning success for those methods would silently fake a playing game.
 */
#define CNK_EGL_SUCCESS 0x3000u
#define CNK_EGL_BAD_ATTRIBUTE 0x3004u
#define CNK_EGL_DISPLAY 1u
#define CNK_EGL_CONFIG 1u
#define CNK_EGL_NONE 0x3038u
static int zeebx_egl_config_attribute(uint32_t attr,uint32_t *out){
 uint32_t value=0;
 switch(attr){
  case 0x3020u: value=16u;break; /* EGL_BUFFER_SIZE */
  case 0x3021u: value=0u;break;  /* ALPHA_SIZE */
  case 0x3022u: case 0x3024u:value=5u;break; /* BLUE/RED */
  case 0x3023u:value=6u;break; /* GREEN_SIZE */
  case 0x3025u:value=16u;break;/* DEPTH_SIZE */
  case 0x3026u:value=8u;break; /* STENCIL_SIZE */
  case 0x3028u:value=CNK_EGL_CONFIG;break;
  case 0x302au:value=480u;break; /* MAX_PBUFFER_HEIGHT */
  case 0x302bu:value=640u*480u;break; /* MAX_PBUFFER_PIXELS */
  case 0x302cu:value=640u;break; /* MAX_PBUFFER_WIDTH */
  case 0x302du:value=1u;break; /* NATIVE_RENDERABLE */
  case 0x3033u:value=0x7u;break; /* SURFACE_TYPE */
  case 0x303fu:value=0x308eu;break; /* COLOR_BUFFER_TYPE RGB */
  case 0x3040u:value=0x1u;break; /* RENDERABLE_TYPE GLES */
  case 0x303bu:case 0x303cu:value=1u;break; /* SWAP_INTERVAL */
  case 0x3027u:case 0x302fu:case 0x3034u:
   value=CNK_EGL_NONE;break;
  case 0x3029u:case 0x302eu:case 0x3031u:case 0x3032u:
  case 0x3035u:case 0x3036u:case 0x3037u:
  case 0x3039u:case 0x303au:
   value=0u;break;
  default:return -1;
 }
 *out=value;
 return 0;
}
static int zeebx_egl_write_at(cnk_zeebx_arm *c,uint32_t ptr,uint32_t val){
 if(ptr && ((ptr&3u)||store(c,ptr,4u,val)))return -3;
 return 0;
}
static int zeebx_egl_intern(cnk_zeebx_arm *c,uint32_t *cache,
                             const char *str){
 if(!*cache){
  size_t len=strlen(str)+1u;
  uint32_t start=(c->heap_cursor+3u)&~3u;
  if(start<c->heap_cursor||len>CNK_ARM_HEAP_BYTES-start)return -2;
  uint32_t ptr=CNK_ARM_HEAP_BASE+start;
  uint8_t *dst=map(c,ptr,len,1);
  if(!dst)return -3;
  memcpy(dst,str,len);
  c->heap_cursor=start+(uint32_t)len;
  *cache=ptr;
 }
 return cnk_zeebx_arm_return_from_api(c,*cache);
}
/* Zeebx AEEGL.h IGL: OLD Common-Lite slots with NO "this" pointer.
 * Unlike synthetic GL success flags, each supported GPU operation calls a
 * bound Vita renderer; no sink or inactive EGL context = explicit API stop.
 * DrawArrays/Elements and textures remain unsupported, and all matrix
 * commands require a real GPU callback before the guest can advance.
 */
static float zeebx_gl_fixed(uint32_t raw){
 return (float)(int32_t)raw/65536.0f;
}
static int zeebx_gl_legacy_call(cnk_zeebx_arm *c,unsigned slot){
 if(!c||!c->initialized||!c->gl_host.present||
    !c->egl_initialized||!c->egl_current_context||
    !c->egl_current_surface)return -4;
 const uint32_t a0=c->regs[0],a1=c->regs[1],
                a2=c->regs[2],a3=c->regs[3];
 int rc=-1;
 switch(slot){
 case 7u:{ /* glClear(GLbitfield) */
  const uint32_t allowed=0x4000u|0x0100u|0x0400u;
  if(a0&~allowed)return -4;
  rc=c->gl_host.clear(c->gl_host_user,a0);
  if(rc)return -4;
  c->gl_clear_calls++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 case 8u:{ /* glClearColorx(GLclampx rgba): signed 16.16 fixed */
  const uint32_t values[4]={a0,a1,a2,a3};
  float rgba[4];
  for(unsigned i=0;i<4u;i++){
   float f=(float)(int32_t)values[i]/65536.0f;
   rgba[i]=f<0.f?0.f:f>1.f?1.f:f;
  }
  rc=c->gl_host.clear_color(c->gl_host_user,rgba[0],rgba[1],
                             rgba[2],rgba[3]);
  if(rc)return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 case 25u: /* glDisableClientState(GL_VERTEX_ARRAY) */
 case 29u: /* glEnableClientState(GL_VERTEX_ARRAY) */
  if(a0!=0x8074u)return -4; /* other client arrays not ported */
  c->gl_vertex_enabled=slot==29u?1u:0u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 26u:{ /* glDrawArrays(GL_TRIANGLES, first, count) */
  if(a0!=4u || (int32_t)a1<0 || !a2 ||
     a2>CNK_ZEEBX_MAX_DRAW_VERTICES || a2%3u ||
     a1>65536u || a2>65536u-a1 ||
     !c->gl_vertex_enabled || !c->gl_vertex_pointer ||
     !c->gl_host.draw_triangles)return -4;
  float xyz[CNK_ZEEBX_MAX_DRAW_VERTICES*3u];
  for(uint32_t i=0;i<a2;i++){
   const uint64_t vertex=(uint64_t)c->gl_vertex_pointer+
                         (uint64_t)(a1+i)*c->gl_vertex_stride;
   for(unsigned component=0;component<3u;component++){
    const uint64_t address=vertex+4u*component;
    uint32_t bits=0;
    if(address>UINT32_MAX ||
       load(c,(uint32_t)address,4u,&bits))return -4;
    float v=0.f;
    memcpy(&v,&bits,sizeof(v));
    if(!isfinite(v))return -4;
    xyz[3u*i+component]=v;
   }
  }
  if(c->gl_host.draw_triangles(c->gl_host_user,xyz,a2))return -4;
  c->gl_draw_calls++;
  c->gl_drawn_vertices+=a2;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 case 37u: /* glGetError: only after supported host calls */
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 46u: /* glLoadIdentity */
  if(!c->gl_host.load_identity ||
     c->gl_host.load_identity(c->gl_host_user))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 51u: /* glMatrixMode, only GLES 1.0 matrix modes */
  if(a0!=0x1700u && a0!=0x1701u && a0!=0x1702u)return -4;
  if(!c->gl_host.matrix_mode ||
     c->gl_host.matrix_mode(c->gl_host_user,a0))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 60u: /* glPopMatrix: GPU checks underflow */
  if(!c->gl_host.pop_matrix ||
     c->gl_host.pop_matrix(c->gl_host_user))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 61u: /* glPushMatrix */
  if(!c->gl_host.push_matrix ||
     c->gl_host.push_matrix(c->gl_host_user))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 63u: /* glRotatex(degrees,x,y,z) as signed 16.16 */
  if(!c->gl_host.rotate ||
     c->gl_host.rotate(c->gl_host_user,zeebx_gl_fixed(a0),
       zeebx_gl_fixed(a1),zeebx_gl_fixed(a2),zeebx_gl_fixed(a3)))
   return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 65u: /* glScalex(x,y,z) */
  if(!c->gl_host.scale ||
     c->gl_host.scale(c->gl_host_user,zeebx_gl_fixed(a0),
                      zeebx_gl_fixed(a1),zeebx_gl_fixed(a2)))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 77u: /* glTranslatex(x,y,z) */
  if(!c->gl_host.translate ||
     c->gl_host.translate(c->gl_host_user,zeebx_gl_fixed(a0),
                          zeebx_gl_fixed(a1),zeebx_gl_fixed(a2)))return -4;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 78u: /* glVertexPointer(size,type,stride,pointer) */
  if(a0!=3u || a1!=0x1406u || a2>256u ||
     (a2 && (a2<12u || (a2&3u))) ||
     !a3 || (a3&3u))return -4;
  c->gl_vertex_pointer=a3;
  c->gl_vertex_stride=a2?a2:12u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 case 79u:{ /* glViewport(x,y,w,h), guest screen 640x480 */
  if((int32_t)a2<0||(int32_t)a3<0||a2>4096u||a3>4096u)
   return -4;
  rc=c->gl_host.viewport(c->gl_host_user,(int32_t)a0,
                          (int32_t)a1,a2,a3);
  if(rc)return -4;
  c->gl_viewport_calls++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 default:return -4;
 }
}

static int zeebx_egl_legacy_call(cnk_zeebx_arm *c,unsigned slot){
 if(!c || !c->initialized)return -1;
 uint32_t a0=c->regs[0],a1=c->regs[1],a2=c->regs[2],a3=c->regs[3];
 c->egl_queries++;
 switch(slot){
 case 3u:{ /* eglGetError() clears the last error. */
  uint32_t err=c->egl_error?c->egl_error:CNK_EGL_SUCCESS;
  c->egl_error=CNK_EGL_SUCCESS;
  return cnk_zeebx_arm_return_from_api(c,err);
 }
 case 4u: /* eglGetDisplay(native_display) -> one Zeebo screen. */
  return cnk_zeebx_arm_return_from_api(c,CNK_EGL_DISPLAY);
 case 5u: /* eglInitialize(display,major*,minor*) */
  if(a0!=CNK_EGL_DISPLAY)return cnk_zeebx_arm_return_from_api(c,0u);
  if((a1&&((a1&3u)||!map(c,a1,4u,1)))||
     (a2&&((a2&3u)||!map(c,a2,4u,1))))return -3;
  if(zeebx_egl_write_at(c,a1,1u)||zeebx_egl_write_at(c,a2,0u))
   return -3;
  c->egl_initialized=1u;
  return cnk_zeebx_arm_return_from_api(c,1u);
 case 6u: /* eglTerminate(display) */
  if(a0!=CNK_EGL_DISPLAY)return cnk_zeebx_arm_return_from_api(c,0u);
  if(c->egl_current_context||c->egl_current_surface)return -4;
  c->egl_initialized=0u;
  c->egl_context=0u;c->egl_surface=0u;
  return cnk_zeebx_arm_return_from_api(c,1u);
 case 7u:{ /* eglQueryString(display,name): no unported extensions advertised */
  if(a0!=CNK_EGL_DISPLAY || !c->egl_initialized)
   return cnk_zeebx_arm_return_from_api(c,0u);
  if(a1==0x3053u)return zeebx_egl_intern(c,&c->egl_string_vendor,"Zeebx");
  if(a1==0x3054u)return zeebx_egl_intern(c,&c->egl_string_version,"1.1");
  if(a1==0x3055u)return zeebx_egl_intern(c,&c->egl_string_extensions,"");
  c->egl_error=CNK_EGL_BAD_ATTRIBUTE;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 case 9u:{ /* eglGetConfigs(display,configs,size,num*) */
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY)
   return cnk_zeebx_arm_return_from_api(c,0u);
  if((a1&&a2&&((a1&3u)||!map(c,a1,4u,1)))||
     (a3&&((a3&3u)||!map(c,a3,4u,1))))return -3;
  if(a1&&a2 && zeebx_egl_write_at(c,a1,CNK_EGL_CONFIG))return -3;
  if(zeebx_egl_write_at(c,a3,a1?(a2?1u:0u):1u))return -3;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 case 10u:{ /* eglChooseConfig(dpy,attribs,cfgs,size,num*) */
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY)
   return cnk_zeebx_arm_return_from_api(c,0u);
  uint32_t count_out=0,matching=1u;
  if(load(c,c->regs[13],4u,&count_out))return -3;
  if((a2&&a3&&((a2&3u)||!map(c,a2,4u,1)))||
     (count_out&&((count_out&3u)||!map(c,count_out,4u,1))))
   return -3;
  if(a1){
   if(a1&3u)return -3;
   int ended=0;
   for(unsigned i=0;i<64u;i++){
    uint32_t attr=0,wanted=0,available=0;
    uint64_t item=(uint64_t)a1+8u*i;
    if(item>UINT32_MAX || load(c,(uint32_t)item,4u,&attr))
     return -3;
    if(attr==CNK_EGL_NONE){ended=1;break;}
    if(item+8u>UINT32_MAX ||
       load(c,(uint32_t)(item+4u),4u,&wanted))return -3;
    if(zeebx_egl_config_attribute(attr,&available)){
     c->egl_error=CNK_EGL_BAD_ATTRIBUTE;
     return cnk_zeebx_arm_return_from_api(c,0u);
    }
    /* Filter bitmasks by containment; min-size attrs by >=. */
    if(attr==0x3033u||attr==0x3040u){
     if((available&wanted)!=wanted)matching=0u;
    }else if(attr==0x3028u||attr==0x303fu){
     if(available!=wanted)matching=0u;
    }else if(available<wanted)matching=0u;
   }
   if(!ended)return -3; /* never walk arbitrary guest memory forever */
  }
  if(a2&&a3&&matching&&zeebx_egl_write_at(c,a2,CNK_EGL_CONFIG))
   return -3;
  if(zeebx_egl_write_at(c,count_out,
                       (a2 ? (a3?matching:0u) : matching)))return -3;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 case 11u:{ /* eglGetConfigAttrib(dpy,cfg,attribute,value*) */
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY||
     a1!=CNK_EGL_CONFIG)return cnk_zeebx_arm_return_from_api(c,0u);
  uint32_t value=0;
  if(zeebx_egl_config_attribute(a2,&value)){
   c->egl_error=CNK_EGL_BAD_ATTRIBUTE;
   return cnk_zeebx_arm_return_from_api(c,0u);
  }
  if(zeebx_egl_write_at(c,a3,value))return -3;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 case 12u:{ /* eglCreateWindowSurface(dpy,config,native_window,attrs)
               * One actual Vita GPU backbuffer; no invented pbuffer. */
  if(!c->gl_host.present)return -4;
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY||
     a1!=CNK_EGL_CONFIG||c->egl_surface)
   return cnk_zeebx_arm_return_from_api(c,0u);
  /* Attribute arrays unsupported until a real interpreter exists.
   * Only EGL_NONE or null is the known original window config. */
  if(a3){
   uint32_t sentinel=0;
   if(load(c,a3,4u,&sentinel))return -3;
   if(sentinel!=CNK_EGL_NONE)return -4;
  }
  c->egl_surface=2u;
  return cnk_zeebx_arm_return_from_api(c,c->egl_surface);
 }
 case 15u:{ /* eglDestroySurface(dpy,surface) */
  if(a0!=CNK_EGL_DISPLAY||a1!=c->egl_surface||!a1||
     a1==c->egl_current_surface)
    return cnk_zeebx_arm_return_from_api(c,0u);
  c->egl_surface=0u;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 case 17u:{ /* eglCreateContext(dpy,cfg,share,attrs) */
  if(!c->gl_host.present)return -4;
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY||
     a1!=CNK_EGL_CONFIG||a2||c->egl_context)
    return cnk_zeebx_arm_return_from_api(c,0u);
  if(a3){
   uint32_t sentinel=0;
   if(load(c,a3,4u,&sentinel))return -3;
   if(sentinel!=CNK_EGL_NONE)return -4;
  }
  c->egl_context=3u;
  return cnk_zeebx_arm_return_from_api(c,c->egl_context);
 }
 case 18u: /* eglDestroyContext */
  if(a0!=CNK_EGL_DISPLAY||!a1||a1!=c->egl_context||
     a1==c->egl_current_context)
   return cnk_zeebx_arm_return_from_api(c,0u);
  c->egl_context=0u;
  return cnk_zeebx_arm_return_from_api(c,1u);
 case 19u:{ /* eglMakeCurrent(dpy,draw,read,context) */
  if(!c->gl_host.present)return -4;
  if(a0!=CNK_EGL_DISPLAY||!c->egl_initialized)
   return cnk_zeebx_arm_return_from_api(c,0u);
  if(!a1&&!a2&&!a3){
   c->egl_current_context=0u;c->egl_current_surface=0u;
   return cnk_zeebx_arm_return_from_api(c,1u);
  }
  if(a1!=c->egl_surface||a2!=c->egl_surface||
     a3!=c->egl_context||!a1||!a3)
   return cnk_zeebx_arm_return_from_api(c,0u);
  if(c->gl_host.make_current(c->gl_host_user,640u,480u))
   return -4;
  c->egl_current_context=a3;c->egl_current_surface=a1;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 case 20u: /* eglGetCurrentContext */
  return cnk_zeebx_arm_return_from_api(c,c->egl_current_context);
 case 21u: /* eglGetCurrentSurface(EGL_DRAW or EGL_READ) */
  return cnk_zeebx_arm_return_from_api(c,c->egl_current_surface);
 case 22u: /* eglGetCurrentDisplay */
  return cnk_zeebx_arm_return_from_api(c,
                                      c->egl_initialized?CNK_EGL_DISPLAY:0u);
 case 26u:{ /* eglSwapBuffers: MUST present a real host frame. */
  if(!c->gl_host.present)return -4;
  if(!c->egl_initialized||a0!=CNK_EGL_DISPLAY||
     a1!=c->egl_surface||!a1||
     c->egl_current_context!=c->egl_context||
     c->egl_current_surface!=a1||!c->egl_context)
   return cnk_zeebx_arm_return_from_api(c,0u);
  if(c->gl_host.present(c->gl_host_user))return -4;
  c->gl_presents++;
  return cnk_zeebx_arm_return_from_api(c,1u);
 }
 default:
  /* Importantly, the GL context, surface and swap calls STOP here.
   * A successful EGL setup does not mean actual graphics run on Vita. */
  return -4;
 }
}

int cnk_zeebx_arm_handle_bootstrap_api(cnk_zeebx_arm *c){
 if(!c||!c->initialized)return -1;
 uint32_t pc=c->regs[15];
 if(pc<CNK_ARM_API_BASE || pc>=CNK_ARM_API_BASE+0x01000000u)
  return -1;
 unsigned iface=(pc-CNK_ARM_API_BASE)>>12u;
 unsigned slot=((pc-CNK_ARM_API_BASE)&0xfffu)/4u;
 /* Old BREW AEEGL.h passes function args directly, not this pointer. */
 if(iface==23u)return zeebx_egl_legacy_call(c,slot);
 if(iface==24u)return zeebx_gl_legacy_call(c,slot);
 /* Zeebx AEEStdLib::memset at helper index 1, as used by the
  * original cnk2.mod while constructing its applet state. Validate
  * writable guest bounds before modifying ANY bytes. */
 if(iface==6u && slot==1u){
  const uint32_t addr=c->regs[0],value=c->regs[1],length=c->regs[2];
  if(length){
   if(length>CNK_ARM_HEAP_BYTES || !map(c,addr,length,1))return -3;
   memset(map(c,addr,length,1),(int)(uint8_t)value,length);
  }
  return cnk_zeebx_arm_return_from_api(c,addr);
 }
 /* Zeebx machine/helper.rs: both AEEStdLib clock entries share the
  * Machine::elapsed_ms virtual time, not host uptime or instruction
  * count. The host controls time progression between frames. */
 if(iface==6u && (slot==43u||slot==44u))
  return cnk_zeebx_arm_return_from_api(c,c->virtual_time_ms);
 if(iface==6u && slot==26u){ /* AEEStdLib malloc */
  uint32_t request=c->regs[0];
  if(request>CNK_ARM_HEAP_BYTES-8u)return -2;
  uint32_t aligned=(request+7u)&~7u;
  if(aligned<8u)aligned=8u;
  if(aligned>CNK_ARM_HEAP_BYTES-c->heap_cursor)return -2;
  uint32_t result=CNK_ARM_HEAP_BASE+c->heap_cursor;
  c->heap_cursor+=aligned;
  return cnk_zeebx_arm_return_from_api(c,result);
 }
 if(iface==0u && (slot==0u||slot==1u)){ /* IShell AddRef/Release */
  if(c->regs[0]!=CNK_ARM_OBJECT_BASE)return -3;
  if(slot==0u){
   if(c->shell_refs==UINT32_MAX)return -3;
   c->shell_refs++;
  }else{
   if(!c->shell_refs)return -3;
   c->shell_refs--;
  }
  return cnk_zeebx_arm_return_from_api(c,c->shell_refs);
 }
 /* Zeebx's ISHELL_GetDeviceInfo (Shell slot 4), ABI from
  * machine/shell.rs. Unlike a guessed "success", this writes the genuine
  * 640x480, 16-bit Zeebo device structure into GUEST memory.
  * Only fields with proven offsets are initialized; extended members
  * require the guest to request the full 64-byte structure at +44. */
 if(iface==0u && slot==4u){
  const uint32_t info=c->regs[1];
  if(c->regs[0]!=CNK_ARM_OBJECT_BASE)return -3;
  if(!info)return cnk_zeebx_arm_return_from_api(c,0u);
  uint32_t requested=0;
  if(load(c,info+44u,2u,&requested))return -3;
  const size_t len=requested>=64u?64u:44u;
  uint8_t *dst=map(c,info,len,1);
  if(!dst)return -3;
  memset(dst,0,len);
  /* First eight uint16 entries of AEEDeviceInfo. */
  putword(dst,640u|(480u<<16));
  dst[8]=8u; /* cxScrollBar */
  dst[10]=3u; /* AEE_ENC_ISOLATIN1 */
  dst[14]=16u; /* COLOR_DEPTH */
  putword(dst+24u,64u*1024u*1024u); /* Zeebx virtual device RAM */
  if(requested>=64u){
   dst[44]=64u; /* wStructSize */
   dst[56]=64u; /* wMaxPath (AEE_MAX_FILE_NAME) */
  }
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 /* Zeebx machine/display.rs: IDISPLAY_GetDeviceBitmap returns an
  * AEE error code (0 for success) and writes IBitmap** to R1.
  * The device bitmap is persistent, not a newly allocated surface on
  * every request. Its drawing methods remain separate API traps.
  * No graphics are fabricated here: the guest interface exists, while
  * GPU output still requires a later Vita display bridge. */
 /* IBitmap::AddRef / Release: the display owns one reference to
  * the persistent screen, even if the game releases its temporary refs.
  * Zeebx machine/bitmap.rs explicitly retains the device bitmap. */
 if(iface==7u && (slot==0u||slot==1u)){
  if(!c->screen_bitmap_addr ||
     c->regs[0]!=c->screen_bitmap_addr)return -3;
  uint32_t count=c->screen_bitmap_references;
  if(slot==0u){
   if(count==UINT32_MAX)return -2;
   count++;
  }else if(count>1u)count--;
  c->screen_bitmap_references=count;
  return cnk_zeebx_arm_return_from_api(c,count);
 }
 /* From Zeebx bitmap.rs QueryInterface. A software IBitmap is also
  * an IDIB if it publishes a real RGB565 framebuffer in guest memory.
  * This is NOT yet linked to Vita's physical scanout. */
 if(iface==7u && slot==2u){
  const uint32_t this_obj=c->regs[0],iid=c->regs[1],out_ptr=c->regs[2];
  uint32_t vtable=0;
  if(!c->screen_bitmap_addr || this_obj!=c->screen_bitmap_addr ||
     load(c,this_obj,4u,&vtable) ||
     vtable!=CNK_ARM_VTABLE_BASE+7u*CNK_ARM_VTABLE_STRIDE ||
     (out_ptr&&!map(c,out_ptr,4u,1)))return -3;
  const int dib=(iid==0x01001045u||iid==0x0100102cu);
  if(!dib&&iid!=0x01001021u)return -4;
  if(dib){
   if(!c->screen_bitmap_pixels){
    const uint32_t bytes=640u*480u*2u; /* native Zeebo RGB565 */
    uint32_t start=(c->heap_cursor+7u)&~7u;
    if(start<c->heap_cursor || bytes>CNK_ARM_HEAP_BYTES-start)return -2;
    uint32_t pixels=CNK_ARM_HEAP_BASE+start;
    if(!map(c,pixels,bytes,1))return -3;
    c->screen_bitmap_pixels=pixels;
    c->heap_cursor=start+bytes;
   }
   /* Layout AEEIDIB: vtable, palette map, pixel pointer, pRGB,
    * transparent key, width, height, row pitch, color count,
    * depth, color scheme, reserved bytes. These offsets are taken
    * from Zeebx bitmap.rs::write_dib_header and AEEIDIB.h. */
   uint8_t *dib_struct=map(c,this_obj,36u,1);
   if(!dib_struct)return -3;
   putword(dib_struct+4u,0u);
   putword(dib_struct+8u,c->screen_bitmap_pixels);
   putword(dib_struct+12u,0u);
   putword(dib_struct+16u,0xf81fu); /* magenta RGB565 color-key */
   putword(dib_struct+20u,640u|(480u<<16));
   putword(dib_struct+24u,1280u); /* pitch + zero RGB count */
   dib_struct[28]=16u;          /* bit depth */
   dib_struct[29]=16u;          /* IDIB_COLORSCHEME_565 */
   memset(dib_struct+30u,0,6u);
  }
  if(c->screen_bitmap_references==UINT32_MAX)return -2;
  if(out_ptr && store(c,out_ptr,4u,this_obj))return -3;
  c->screen_bitmap_references++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface==5u && slot==16u){
  uint32_t disp=c->regs[0],out_ptr=c->regs[1],vtbl=0;
  if(!out_ptr || load(c,disp,4u,&vtbl) ||
     vtbl!=CNK_ARM_VTABLE_BASE+5u*CNK_ARM_VTABLE_STRIDE ||
     !map(c,out_ptr,4u,1))return -3;
  if(!c->screen_bitmap_addr){
   if(c->object_cursor>OBJECT_BYTES-64u)return -2;
   c->screen_bitmap_addr=CNK_ARM_OBJECT_BASE+c->object_cursor;
   if(store(c,c->screen_bitmap_addr,4u,
            CNK_ARM_VTABLE_BASE+7u*CNK_ARM_VTABLE_STRIDE))return -3;
   c->object_cursor+=64u;
   c->screen_bitmap_references=1u; /* Zeebx display retains ownership */
  }
  if(c->screen_bitmap_references==UINT32_MAX)return -2;
  if(store(c,out_ptr,4u,c->screen_bitmap_addr))return -3;
  c->screen_bitmap_references++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 /* Zeebx machine/hid.rs with ONE console controller provided by Vita.
  * The actual button event/position bridge is not connected yet.
  * GetConnectedDevices has a 5th AAPCS parameter on the guest stack:
  * handle type (r1), handle array (r2), array capacity (r3), needed
  * count (sp+0). Validate every guest write before changing memory. */
 if(iface==8u && slot==7u){ /* IHID::GetConnectedDevices */
  const uint32_t wanted=c->regs[1],handles=c->regs[2],capacity=c->regs[3];
  uint32_t needed_ptr=0;
  if(load(c,c->regs[13],4u,&needed_ptr))return -3;
  const uint32_t found=wanted==0x0106c3fdu?1u:0u;
  if((needed_ptr&&!map(c,needed_ptr,4u,1))||
     (found&&capacity&&handles&&!map(c,handles,4u,1)))
   return -3;
  if(needed_ptr && store(c,needed_ptr,4u,found))return -3;
  if(found&&capacity&&handles && store(c,handles,4u,1u))return -3;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface==8u && slot==3u){ /* IHID::CreateDevice */
  const uint32_t handle=c->regs[1],out=c->regs[2];
  uint32_t vtbl=0;
  if(handle!=1u||!out || load(c,c->regs[0],4u,&vtbl)||
     vtbl!=CNK_ARM_VTABLE_BASE+8u*CNK_ARM_VTABLE_STRIDE ||
     !map(c,out,4u,1))return -3;
  if(c->object_cursor>OBJECT_BYTES-64u)return -2;
  const uint32_t dev=CNK_ARM_OBJECT_BASE+c->object_cursor;
  if(store(c,dev,4u,CNK_ARM_VTABLE_BASE+9u*CNK_ARM_VTABLE_STRIDE)||
     store(c,out,4u,dev))return -3;
  c->object_cursor+=64u;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 /* 12 bytes: AEEUID joystick, USB product ID, USB vendor ID,
  * bluetooth boolean, sourced from Zeebx's Zeebo controller descriptor. */
 if((iface==8u && slot==4u)||(iface==9u && slot==3u)){
  uint32_t vtbl=0;
  const uint32_t obj=c->regs[0];
  const uint32_t out=iface==8u?c->regs[2]:c->regs[1];
  const uint32_t handle=iface==8u?c->regs[1]:1u;
  if(handle!=1u||load(c,obj,4u,&vtbl)||
     vtbl!=CNK_ARM_VTABLE_BASE+iface*CNK_ARM_VTABLE_STRIDE||
     (out&&!map(c,out,12u,1)))return -3;
  if(out){
   uint8_t *dst=map(c,out,12u,1);
   putword(dst,0x0106c3fdu);
   dst[4]=0x35u;dst[5]=0x01u; /* product 0x0135 */
   dst[6]=0xaau;dst[7]=0x1eu; /* vendor 0x1eaa */
   putword(dst+8u,0u);
  }
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if((iface==12u&&slot==3u) ||
    ((iface==10u||iface==11u)&&slot<=5u))
  return zeebx_signal_api(c,iface,slot);
 if(iface==9u&&(slot==8u||slot==14u))
  return zeebx_register_pad_signal(c,slot);
 if(iface==9u && (slot==6u||slot==7u||slot==9u||
                   (slot>=10u&&slot<=13u)))
  return zeebx_device_input(c,slot);
 if(iface==9u && slot==4u){ /* IHIDDevice::GetDeviceStatus */
  uint32_t vtbl=0;
  const uint32_t out=c->regs[1];
  if(load(c,c->regs[0],4u,&vtbl)||
     vtbl!=CNK_ARM_VTABLE_BASE+9u*CNK_ARM_VTABLE_STRIDE||
     (out&&!map(c,out,4u,1)))return -3;
  if(out&&store(c,out,4u,1u))return -3; /* CONNECTED */
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 if(iface==8u&&slot==5u){ /* no new connection events */
  for(unsigned i=1u;i<=3u;i++)if(c->regs[i]&&!map(c,c->regs[i],4u,1))
   return -3;
  for(unsigned i=1u;i<=3u;i++)if(c->regs[i] &&
      store(c,c->regs[i],4u,0u))return -3;
  return cnk_zeebx_arm_return_from_api(c,1u); /* BREW EFAILED = empty */
 }
 /* The original CNK applet reaches this exact diagnostic helper twice.
  * Zeebx returns AEE_SUCCESS after recording formatted debug text.
  * Preserve the bounded raw format string without interpreting guest
  * varargs as a host printf format (unsafe and not the same ABI). */
 if(iface==6u&&slot==39u){
  uint32_t ptr=c->regs[0],byte=0;
  if(!ptr||c->debug_calls==UINT32_MAX)return -3;
  size_t n=0;
  memset(c->last_debug_template,0,sizeof(c->last_debug_template));
  for(;n+1u<sizeof(c->last_debug_template);n++){
   if(load(c,ptr+(uint32_t)n,1u,&byte))return -3;
   if(!byte)break;
   c->last_debug_template[n]=(char)(uint8_t)byte;
  }
  c->last_debug_template[n]='\0';
  c->debug_calls++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 /* Zeebx Machine::shell_create_instance and brew/aee.rs:
  * Crash's class scan (docs/varredura in Zeebx upstream) records HID,
  * ISignalCBFactory, legacy AEEGL EGL/GL, Display, FileMgr and Sound.
  * Create a genuine guest-side object/vtable for the exact class, but
  * NEVER return fake success for methods not implemented yet. */
 if(iface==0u&&slot==2u){
  const uint32_t clsid=c->regs[1],out_ptr=c->regs[2];
  uint32_t target_iface=0;
  if(clsid==0x01001001u)target_iface=5u; /* IDisplay */
  else if(clsid==0x01001003u)target_iface=3u; /* IFileMgr */
  else if(clsid==0x01001056u)target_iface=14u; /* ISound */
  else if(clsid==0x0106c411u)target_iface=8u;  /* Zeebo IHID */
  else if(clsid==0x01041207u)target_iface=12u; /* ISignalCBFactory */
  else if(clsid==0x01014bc4u)target_iface=23u; /* legacy EGL, AEEGL.h */
  else if(clsid==0x01014bc3u)target_iface=24u; /* legacy GL, AEEGL.h */
  else return -4;
  if(c->regs[0]!=CNK_ARM_OBJECT_BASE||!out_ptr||
     !map(c,out_ptr,4u,1))return -3;
  if(c->object_cursor>OBJECT_BYTES-64u)return -2;
  const uint32_t obj=CNK_ARM_OBJECT_BASE+c->object_cursor;
  const uint32_t vtable=CNK_ARM_VTABLE_BASE+
                         target_iface*CNK_ARM_VTABLE_STRIDE;
  if(store(c,obj,4u,vtable)||store(c,out_ptr,4u,obj))return -3;
  c->object_cursor+=64u;
  if(target_iface==5u)c->display_objects++;
  else if(target_iface==3u)c->filemgr_objects++;
  else if(target_iface==14u)c->sound_objects++;
  else if(target_iface==8u)c->hid_objects++;
  else if(target_iface==12u)c->signal_factory_objects++;
  else if(target_iface==23u)c->egl_legacy_objects++;
  else if(target_iface==24u)c->gl_legacy_objects++;
  return cnk_zeebx_arm_return_from_api(c,0u);
 }
 return -4; /* Unsupported API MUST NOT silently return success. */
}
int cnk_zeebx_arm_bootstrap(cnk_zeebx_arm *c,uint32_t budget,
                            cnk_arm_outcome *out,uint32_t *apis_handled){
 if(!c||!out||!apis_handled||!c->initialized||!budget||
    budget>10000000u)return -1;
 *apis_handled=0;
 const uint64_t start=c->executed;
 for(;;){
  uint64_t used=c->executed-start;
  if(used>=budget){
   *out=(cnk_arm_outcome){0};
   out->reason=CNK_ARM_BUDGET;
   out->pc=c->regs[15];
   out->executed=c->executed;
   return 0;
  }
  uint32_t left=(uint32_t)(budget-used);
  if(left>4096u)left=4096u;
  cnk_arm_outcome step_out={0};
  if(cnk_zeebx_arm_run(c,left,&step_out))return -2;
  if(step_out.reason==CNK_ARM_API){
   if(cnk_zeebx_arm_handle_bootstrap_api(c)){
    *out=step_out;return 0; /* real unknown BREW API */
   }
   if(*apis_handled==UINT32_MAX)return -3;
   (*apis_handled)++;
   /* Zero-instruction API loops are bounded by the requested budget. */
   if(*apis_handled>10000u){*out=step_out;return 0;}
   continue;
  }
  if(step_out.reason!=CNK_ARM_BUDGET){
   *out=step_out;return 0;
  }
 }
}

/* Zeebx machine/signal.rs: deferred guest callback delivery, only AFTER
 * the original guest has returned to its host. Reentering while executing
 * BREW dispatch would corrupt ARM AAPCS registers and game callbacks.
 * Neither an unknown callback address nor an unresolved BREW API may be
 * interpreted as success. All callbacks are bounded by the caller's budget.
 */
int cnk_zeebx_arm_deliver_signals(cnk_zeebx_arm *c,uint32_t budget,
                                  cnk_arm_outcome *out,uint32_t *delivered){
 if(!c||!out||!delivered||!c->initialized||!budget||
    budget>10000000u||c->regs[15]!=CNK_ARM_RETURN_MAGIC)
  return -1;
 *delivered=0u;
 memset(out,0,sizeof(*out));
 const uint64_t start=c->executed;
 while(c->signal_pending_count && c->executed-start<budget){
  const cnk_zeebx_signal_callback cb=
   c->signal_pending[c->signal_pending_head];
  const uint32_t code=cb.function&~1u;
  const uint64_t exec_end=(uint64_t)CNK_ARM_MOD_BASE+
                           c->module_size-CNK_ARM_MOD_PREFIX;
  /* The adapted interpreter does not yet execute guest THUMB.
   * A malformed function pointer stops BEFORE touching registers. */
  if((cb.function&1u)||(code&3u)||code<CNK_ARM_MOD_BASE||
     (uint64_t)code+4u>exec_end){
   out->reason=CNK_ARM_UNSUPPORTED;
   out->pc=cb.function;
   out->executed=c->executed;
   return -2;
  }
  uint32_t saved_regs[16];
  memcpy(saved_regs,c->regs,sizeof(saved_regs));
  const uint32_t saved_cpsr=c->cpsr;
  c->regs[0]=cb.context;
  c->regs[1]=0u;c->regs[2]=0u;c->regs[3]=0u;
  c->regs[14]=CNK_ARM_RETURN_MAGIC;
  c->regs[15]=code;
  c->cpsr&=~T;
  cnk_arm_outcome call_out={0};
  uint32_t apis_handled=0u;
  int rc=cnk_zeebx_arm_bootstrap(
   c,(uint32_t)(budget-(c->executed-start)),&call_out,&apis_handled);
  /* When BX LR is the last budgeted guest ARM instruction, bootstrap
   * reports BUDGET before performing another sentinel check. The
   * instruction HAS reached the return sentinel, so count this callback
   * as completed instead of losing its event or replaying it next time. */
  if(!rc && call_out.reason==CNK_ARM_BUDGET &&
     c->regs[15]==CNK_ARM_RETURN_MAGIC)
    call_out.reason=CNK_ARM_RETURNED;
  /* Host-owned saved ARM registers remain untouched even on faults.
   * Any guest-memory writes persist, just like real callback execution. */
  memcpy(c->regs,saved_regs,sizeof(saved_regs));
  c->cpsr=saved_cpsr;
  if(rc||call_out.reason!=CNK_ARM_RETURNED){
   *out=call_out;
   return rc?-3:-2;
  }
  c->signal_pending_head=(c->signal_pending_head+1u)%
                          CNK_ZEEBX_PENDING_SIGNALS;
  c->signal_pending_count--;
  (*delivered)++;
 }
 out->reason=c->signal_pending_count?CNK_ARM_BUDGET:CNK_ARM_RETURNED;
 out->pc=c->regs[15];
 out->executed=c->executed;
 return 0;
}

/* Stage 2 reproduced from Zeebx machine/shell.rs::create_applet:
 * read the guest's actual module pointer and vtable, then call
 * CreateInstance slot #2 using the ARM AAPCS (r0..r3).
 * This neither fabricates a class nor calls host-native code. */
int cnk_zeebx_arm_prepare_applet(cnk_zeebx_arm *c,uint32_t clsid,
                                  uint32_t *entry){
 uint32_t module_ptr=0,vtable=0,func=0;
 if(!c||!entry||!c->initialized||c->applet_pending||
    c->regs[15]!=CNK_ARM_RETURN_MAGIC || c->regs[0]!=0u||
    !clsid)return -1;
 *entry=0;
 if(load(c,CNK_ARM_OBJECT_BASE+8u,4u,&module_ptr))return -2;
 /* The Zeebx bootstrap malloc is the only source of a valid
  * allocated guest module object at this point. */
 if(module_ptr<CNK_ARM_HEAP_BASE || (module_ptr&3u) ||
    (uint64_t)module_ptr+4u>
      (uint64_t)CNK_ARM_HEAP_BASE+c->heap_cursor)return -3;
 if(load(c,module_ptr,4u,&vtable) || (vtable&3u) ||
    load(c,vtable+8u,4u,&func))return -4;
 /* Only source-backed module code can be executed by this subset;
  * don't silently treat an API marker or random heap as applet code. */
 uint32_t aligned=func&~1u;
 uint64_t code_end=(uint64_t)CNK_ARM_MOD_BASE+c->module_size-
                       CNK_ARM_MOD_PREFIX;
 if(aligned<CNK_ARM_MOD_BASE || (uint64_t)aligned+4u>code_end ||
    (aligned&3u))return -5;
 /* Clear the out IApplet** as Zeebx does, not module output. */
 if(store(c,CNK_ARM_OBJECT_BASE+12u,4u,0u))return -6;
 c->regs[0]=module_ptr;
 c->regs[1]=CNK_ARM_OBJECT_BASE;
 c->regs[2]=clsid;
 c->regs[3]=CNK_ARM_OBJECT_BASE+12u;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 if(func&1u)c->cpsr|=T;
 else c->cpsr&=~T;
 c->regs[15]=aligned;
 c->applet_pending=1;
 *entry=func;
 return 0;
}
int cnk_zeebx_arm_applet_result(cnk_zeebx_arm *c,
                                const cnk_arm_outcome *stop,
                                uint32_t *applet_ptr){
 uint32_t applet=0,vtable=0;
 if(!c||!c->initialized||!c->applet_pending||
    !stop||!applet_ptr)return -1;
 *applet_ptr=0;
 if(stop->reason!=CNK_ARM_RETURNED ||
    c->regs[15]!=CNK_ARM_RETURN_MAGIC || c->regs[0]!=0u)
  return -2;
 if(load(c,CNK_ARM_OBJECT_BASE+12u,4u,&applet) || !applet)
  return -3;
 if(load(c,applet,4u,&vtable) || !vtable)
  return -4;
 /* The pointer must refer to guest memory. The normal ARM fetch path
  * will independently validate any attempted function invocation. */
 if(!map(c,vtable,4u,0))return -5;
 c->applet_pending=0;
 *applet_ptr=applet;
 return 0;
}
