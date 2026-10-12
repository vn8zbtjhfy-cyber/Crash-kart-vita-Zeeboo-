/* Zeebx BREW IFileMgr::GetInfo and IFile::GetInfo ABI regression.
 * Original game data is NEVER needed. Test uses a disposable fixture.
 * AEEFileInfo: 0 attribute, 4 reserved, 8 file length, 12..75 name.
 */
#include "cnk_zeebx_arm.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ CNK_ARM_OBJECT_BASE
#define HEAP CNK_ARM_HEAP_BASE
static void call(cnk_zeebx_arm *cpu,uint32_t obj,unsigned iface,
                 unsigned slot,uint32_t p1,uint32_t p2){
 cpu->regs[0]=obj;cpu->regs[1]=p1;cpu->regs[2]=p2;cpu->regs[3]=0u;
 cpu->regs[14]=CNK_ARM_RETURN_MAGIC;
 cpu->regs[15]=CNK_ARM_API_BASE+(iface<<12)+4u*slot;
}
static uint32_t rd(cnk_zeebx_arm *cpu,uint32_t addr){
 uint32_t x=0;
 assert(!cnk_zeebx_arm_read_u32(cpu,addr,&x));
 return x;
}
static void check_info(cnk_zeebx_arm *cpu,uint32_t addr,
                       const char *name,unsigned size){
 assert(rd(cpu,addr)==0u);       /* FA_NORMAL */
 assert(rd(cpu,addr+4u)==0u);   /* reserved */
 assert(rd(cpu,addr+8u)==size); /* real size */
 assert(!strcmp((const char*)cpu->heap+(addr-HEAP)+12u,name));
 size_t n=strlen(name);
 assert(n<64u);
 for(size_t i=n+1u;i<64u;i++)
  assert(cpu->heap[(addr-HEAP)+12u+i]==0u);
}
int main(int argc,char **argv){
 assert(argc==2&&argv[1]);
 const char *name="cnk_zeebx_info_fixture.dat";
 char host_path[512];
 int n=snprintf(host_path,sizeof(host_path),"%s/%s",argv[1],name);
 assert(n>0&&(size_t)n<sizeof(host_path));
 const uint8_t fixture[]={9,8,7,0xFF,3,2,1,0,4,5,6,7};
 FILE *file=fopen(host_path,"wb");assert(file);
 assert(fwrite(fixture,1,sizeof(fixture),file)==sizeof(fixture));
 assert(!fclose(file));

 const uint8_t mod[128]={0};
 cnk_zeebx_arm cpu={0};
 assert(!cnk_zeebx_arm_init(&cpu,mod,sizeof(mod)));
 assert(!cnk_zeebx_arm_set_file_root(&cpu,argv[1]));
 call(&cpu,OBJ,0u,2u,0x01001003u,OBJ+0x200u);
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 uint32_t mgr=rd(&cpu,OBJ+0x200u);
 assert(mgr && rd(&cpu,mgr)==
        CNK_ARM_VTABLE_BASE+3u*CNK_ARM_VTABLE_STRIDE);

 strcpy((char*)cpu.heap+32u,name);
 const uint32_t out=HEAP+320u;
 memset(cpu.heap+320u,0xCA,76u);
 call(&cpu,mgr,3u,3u,HEAP+32u,out);  /* IFileMgr::GetInfo */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==0u);
 check_info(&cpu,out,name,sizeof(fixture));
 assert(cpu.file_open_count==0u&&cpu.file_read_calls==0u);
 assert(cpu.heap[319u]==0u);
 assert(cpu.heap[396u]==0u);

 /* Prevalidate 76 contiguous guest bytes without partial modification. */
 const uint32_t short_out=HEAP+CNK_ARM_HEAP_BYTES-40u;
 cpu.heap[CNK_ARM_HEAP_BYTES-40u]=0x7Fu;
 call(&cpu,mgr,3u,3u,HEAP+32u,short_out);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-3);
 assert(cpu.heap[CNK_ARM_HEAP_BYTES-40u]==0x7Fu);
 call(&cpu,mgr,3u,3u,HEAP+32u,out+1u);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-3);
 call(&cpu,mgr,3u,3u,HEAP+32u,0u); /* status-only, no struct */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==0u);

 call(&cpu,mgr,3u,2u,HEAP+32u,0u); /* OpenFile, read-only */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 uint32_t guest_file=cpu.regs[0];
 assert(guest_file && cpu.file_open_count==1u);
 call(&cpu,guest_file,4u,3u,HEAP+210u,3u); /* Read */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==3u);
 assert(!memcmp(cpu.heap+210u,fixture,3u));
 memset(cpu.heap+320u,0xCA,76u);
 call(&cpu,guest_file,4u,6u,out,0u); /* IFile::GetInfo */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==0u);
 check_info(&cpu,out,name,sizeof(fixture));
 call(&cpu,guest_file,4u,7u,2u,0u); /* SEEK_CURRENT zero */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==3u); /* GetInfo did not change read position */
 call(&cpu,guest_file,4u,6u,short_out,0u);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-3);
 call(&cpu,guest_file,4u,6u,0u,0u);
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==0u);

 /* Missing files give EFAILED and NEVER overwrite stale output info. */
 strcpy((char*)cpu.heap+32u,"missing-test-only-148.dat");
 cpu.heap[320u]=0x51u;
 call(&cpu,mgr,3u,3u,HEAP+32u,out);
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.regs[0]==1u && cpu.heap[320u]==0x51u);
 /* Same sandbox for metadata as OpenFile: no global paths/traversal. */
 strcpy((char*)cpu.heap+32u,"../outside.dat");
 call(&cpu,mgr,3u,3u,HEAP+32u,out);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-4);
 assert(cpu.file_path_denials==1u);
 strcpy((char*)cpu.heap+32u,"fs:/shared/other.dat");
 call(&cpu,mgr,3u,3u,HEAP+32u,out);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-4);
 assert(cpu.file_path_denials==2u);

 call(&cpu,guest_file,4u,1u,0u,0u); /* Release */
 assert(!cnk_zeebx_arm_handle_bootstrap_api(&cpu));
 assert(cpu.file_open_count==0u);
 call(&cpu,guest_file,4u,6u,out,0u);
 assert(cnk_zeebx_arm_handle_bootstrap_api(&cpu)==-4);
 cnk_zeebx_arm_destroy(&cpu);
 file=fopen(host_path,"rb");assert(file);
 uint8_t still_original[sizeof(fixture)]={0};
 assert(fread(still_original,1,sizeof(still_original),file)==sizeof(fixture));
 assert(!fclose(file));
 assert(!memcmp(still_original,fixture,sizeof(fixture)));
 assert(!remove(host_path));
 puts("PASS: Zeebx BREW GetInfo 76-byte real metadata, cursor, missing file,"
      " guest bounds, immutable private game root");
 return 0;
}
