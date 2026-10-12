/* Original Zeebx IFileMgr/IFile guest ABI. ROM-free, synthetic file.
 * Tests real guest-memory transfers and bounded/private host file access.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define OBJ (CNK_ARM_OBJECT_BASE)
#define HEAP (CNK_ARM_HEAP_BASE)
static void run_api(cnk_zeebx_arm *c,uint32_t obj,uint32_t iface,
                    uint32_t slot,uint32_t a1,uint32_t a2){
 c->regs[0]=obj;c->regs[1]=a1;c->regs[2]=a2;c->regs[3]=0u;
 c->regs[14]=CNK_ARM_RETURN_MAGIC;
 c->regs[15]=CNK_ARM_API_BASE+(iface<<12)+4u*slot;
}
static int invoke(cnk_zeebx_arm *c){
 return cnk_zeebx_arm_handle_bootstrap_api(c);
}
static uint32_t guest_word(cnk_zeebx_arm *c,uint32_t addr){
 uint32_t v=0;
 assert(!cnk_zeebx_arm_read_u32(c,addr,&v));return v;
}
int main(int argc,char **argv){
 assert(argc==2&&argv[1]&&strlen(argv[1])<240u);
 char filename[320];
 assert(snprintf(filename,sizeof(filename),"%s/%s",argv[1],
                 "cnk_zeebx_synthetic.dat")>0);
 const uint8_t fixture[19]={
  'Z','E','E','B','X',0,0x01,0xfe,9,8,7,6,5,4,3,2,1,0,0xff
 };
 FILE *f=fopen(filename,"wb");assert(f);
 assert(fwrite(fixture,1,sizeof(fixture),f)==sizeof(fixture));
 assert(!fclose(f));
 uint8_t synthetic_mod[128]={0};
 cnk_zeebx_arm cpu={0};
 assert(!cnk_zeebx_arm_init(&cpu,synthetic_mod,sizeof(synthetic_mod)));
 /* Get a true IFileMgr guest object from original Zeebx Shell factory. */
 run_api(&cpu,OBJ,0u,2u,0x01001003u,OBJ+0x200u);
 assert(!invoke(&cpu));
 uint32_t mgr=guest_word(&cpu,OBJ+0x200u);
 assert(mgr>=OBJ+16u);
 assert(guest_word(&cpu,mgr)==CNK_ARM_VTABLE_BASE+3u*CNK_ARM_VTABLE_STRIDE);
 assert(!cpu.file_open_count);
 /* The probe must not read host files until explicitly bound. */
 strcpy((char*)cpu.heap+32u,"cnk_zeebx_synthetic.dat");
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(invoke(&cpu)==-4);
 assert(!cnk_zeebx_arm_set_file_root(&cpu,argv[1]));
 /* Opening the exact synthetic guest resource creates a real IFile object. */
 run_api(&cpu,mgr,3u,7u,HEAP+32u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u); /* Test returns BREW SUCCESS */
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(!invoke(&cpu));
 uint32_t file=cpu.regs[0];
 assert(file!=0u&&file!=mgr);
 assert(cpu.file_open_count==1u);
 assert(guest_word(&cpu,file)==CNK_ARM_VTABLE_BASE+4u*CNK_ARM_VTABLE_STRIDE);
 assert(cpu.file_read_calls==0u);
 run_api(&cpu,file,4u,9u,HEAP+120u,0u); /* real GetInfoEx layout */
 assert(!invoke(&cpu)&&cpu.regs[0]==0u);
 for(unsigned i=120u;i<132u;i++)assert(cpu.heap[i]==0u);
 assert(guest_word(&cpu,HEAP+132u)==sizeof(fixture));
 run_api(&cpu,file,4u,3u,HEAP+256u,5u);
 assert(!invoke(&cpu)&&cpu.regs[0]==5u);
 assert(!memcmp(cpu.heap+256u,fixture,5u));
 assert(cpu.file_read_calls==1u&&cpu.file_bytes_read==5u);
 run_api(&cpu,file,4u,7u,2u,0u); /* SEEK_CURRENT zero gives offset */
 assert(!invoke(&cpu)&&cpu.regs[0]==5u);
 run_api(&cpu,file,4u,3u,HEAP+270u,32u); /* short read at EOF */
 assert(!invoke(&cpu)&&cpu.regs[0]==sizeof(fixture)-5u);
 assert(!memcmp(cpu.heap+270u,fixture+5u,sizeof(fixture)-5u));
 run_api(&cpu,file,4u,3u,HEAP+256u,5u);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u); /* true EOF */
 run_api(&cpu,file,4u,7u,0u,2u);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u);
 run_api(&cpu,file,4u,3u,HEAP+300u,3u);
 assert(!invoke(&cpu)&&cpu.regs[0]==3u);
 assert(!memcmp(cpu.heap+300u,fixture+2u,3u));
 run_api(&cpu,file,4u,7u,1u,(uint32_t)(int32_t)-2);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u);
 run_api(&cpu,file,4u,3u,HEAP+310u,3u);
 assert(!invoke(&cpu)&&cpu.regs[0]==2u);
 assert(cpu.heap[310u]==0u&&cpu.heap[311u]==0xffu);
 /* Reject invalid guest destination BEFORE reading/changing file cursor. */
 run_api(&cpu,file,4u,7u,0u,0u);
 assert(!invoke(&cpu));
 run_api(&cpu,file,4u,3u,0xffffffffu,6u);
 assert(invoke(&cpu)==-3);
 run_api(&cpu,file,4u,3u,HEAP+350u,6u);
 assert(!invoke(&cpu)&&cpu.regs[0]==6u);
 assert(!memcmp(cpu.heap+350u,fixture,6u));
 run_api(&cpu,file,4u,3u,HEAP+350u,CNK_ZEEBX_MAX_FILE_READ+1u);
 assert(invoke(&cpu)==-4);
 /* Writing to original game files is not silently implemented. */
 run_api(&cpu,file,4u,5u,HEAP+350u,6u);
 assert(invoke(&cpu)==-4);
 run_api(&cpu,mgr,3u,2u,HEAP+32u,4u); /* OFM_CREATE */
 assert(invoke(&cpu)==-4);
 run_api(&cpu,mgr,3u,2u,HEAP+32u,2u); /* OFM_READWRITE */
 assert(invoke(&cpu)==-4);
 /* No guest directory traversal or absolute host paths. */
 strcpy((char*)cpu.heap+32u,"../cnk_zeebx_synthetic.dat");
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(invoke(&cpu)==-4);
 strcpy((char*)cpu.heap+32u,"/etc/passwd");
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(invoke(&cpu)==-4);
 strcpy((char*)cpu.heap+32u,"fs:/shared/something");
 run_api(&cpu,mgr,3u,7u,HEAP+32u,0u);
 assert(invoke(&cpu)==-4);
 assert(cpu.file_path_denials==3u);
 /* Missing file returns NULL like original Zeebx; not fake success. */
 strcpy((char*)cpu.heap+32u,"missing.bin");
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u);
 /* Zeebo's module-relative prefix is accepted, always within root. */
 strcpy((char*)cpu.heap+32u,"fs:/~/cnk_zeebx_synthetic.dat");
 run_api(&cpu,mgr,3u,2u,HEAP+32u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]!=0u);
 uint32_t second=cpu.regs[0];
 assert(cpu.file_open_count==2u);
 /* AddRef + Release must not close while an owner remains. */
 run_api(&cpu,file,4u,0u,0u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]==2u);
 run_api(&cpu,file,4u,1u,0u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]==1u);
 run_api(&cpu,file,4u,1u,0u,0u);
 assert(!invoke(&cpu)&&cpu.regs[0]==0u);
 assert(cpu.file_open_count==1u);
 run_api(&cpu,file,4u,3u,HEAP+310u,3u);
 assert(invoke(&cpu)==-4); /* use-after-close denied */
 run_api(&cpu,second,4u,1u,0u,0u);
 assert(!invoke(&cpu)&&cpu.file_open_count==0u);
 /* Detached game root can be changed only after file releases. */
 assert(!cnk_zeebx_arm_set_file_root(&cpu,argv[1]));
 cnk_zeebx_arm_destroy(&cpu);
 f=fopen(filename,"rb");assert(f);
 uint8_t untouched[sizeof(fixture)]={0};
 assert(fread(untouched,1,sizeof(untouched),f)==sizeof(untouched));
 assert(!fclose(f));
 assert(!memcmp(untouched,fixture,sizeof(fixture)));
 assert(!remove(filename));
 puts("PASS: original Zeebx IFileMgr/OpenFile Test, read/seek/GetInfoEx,"
      " directory sandbox, immutable files and bounded guest memory");
 return 0;
}
