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
  putword(cpu->vtables+14u*CNK_ARM_VTABLE_STRIDE+4u*i,
          CNK_ARM_API_BASE+(14u<<12)+i*4u);
  putword(cpu->helpers+4u*i,CNK_ARM_API_BASE+(6u<<12)+i*4u);
 }
 cpu->regs[0]=CNK_ARM_OBJECT_BASE;
 cpu->regs[1]=CNK_ARM_HELPERS_BASE;
 cpu->regs[2]=CNK_ARM_OBJECT_BASE+8u;
 cpu->regs[13]=CNK_ARM_STACK_BASE+CNK_ARM_STACK_BYTES-16u;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_MOD_BASE+info.entry_offset;
 cpu->cpsr=0x10u; /* user mode ARM */
 cpu->heap_cursor=0u;
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
int cnk_zeebx_arm_handle_bootstrap_api(cnk_zeebx_arm *c){
 if(!c||!c->initialized)return -1;
 uint32_t pc=c->regs[15];
 if(pc<CNK_ARM_API_BASE || pc>=CNK_ARM_API_BASE+0x01000000u)
  return -1;
 unsigned iface=(pc-CNK_ARM_API_BASE)>>12u;
 unsigned slot=((pc-CNK_ARM_API_BASE)&0xfffu)/4u;
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
 /* Zeebx Machine::shell_create_instance: these ClassIDs come from the
  * BREW 4 SDK and were individually observed when executing the original
  * user-owned cnk2.mod. No generic success for unknown classes. */
 if(iface==0u&&slot==2u){
  const uint32_t clsid=c->regs[1],out_ptr=c->regs[2];
  uint32_t target_iface=0;
  if(clsid==0x01001001u)target_iface=5u; /* IDisplay */
  else if(clsid==0x01001003u)target_iface=3u; /* IFileMgr */
  else if(clsid==0x01001056u)target_iface=14u; /* ISound */
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
  else c->sound_objects++;
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
