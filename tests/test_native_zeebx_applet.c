/* Synthetic ARM guest program modeling Zeebx's two-stage lifecycle.
 * This is not a commercial game binary or a fabricated applet in native C.
 * Real guest ARM writes its IModule and IApplet vtable/pointers itself. */
#include "cnk_zeebx_arm.h"
#include "cnk_zeebx_probe.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void wr32(uint8_t *p,uint32_t x){
 for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
int main(void){
 uint8_t code[256]={0};
 /* AEEMod_Load: malloc 36, store IModule*, assign guest vtable. */
 wr32(code+0x00,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+0x04,0xe1a05000u); /* MOV r5,r0 */
 wr32(code+0x08,0xe1a06002u); /* MOV r6,r2 */
 wr32(code+0x0c,0xe3a00024u); /* MOV r0,#36 */
 wr32(code+0x10,0xe5917068u); /* LDR r7,[r1,#0x68] malloc */
 wr32(code+0x14,0xe12fff37u); /* BLX r7 */
 wr32(code+0x18,0xe5860000u); /* STR r0,[r6] */
 wr32(code+0x1c,0xe59f301cu); /* LDR r3,[pc,#0x1c] -> literal 0x40 */
 wr32(code+0x20,0xe5803000u); /* STR r3,[r0] vtable */
 wr32(code+0x24,0xe3a00000u); /* MOV r0,#0 */
 wr32(code+0x28,0xe1a0e004u); /* MOV lr,r4 */
 wr32(code+0x2c,0xe12fff1eu); /* BX lr */
 wr32(code+0x40,CNK_ARM_MOD_BASE+0xc0u);
 /* Guest module vtable slot 2 (IModule::CreateInstance). */
 wr32(code+0xc8,CNK_ARM_MOD_BASE+0x60u);
 /* Stage 2: CreateInstance(IModule*, IShell*, ClassID, IApplet**).
  * Reuse Zeebx's helper trampoline to allocate 20 guest bytes. */
 wr32(code+0x60,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+0x64,0xe1a05003u); /* MOV r5,r3, output IApplet** */
 wr32(code+0x68,0xe3a00014u); /* MOV r0,#20 before guest malloc */
 wr32(code+0x6c,0xe59f701cu); /* LDR r7,[pc,#0x1c] helper at 0x90 */
 wr32(code+0x70,0xe12fff37u); /* BLX r7 */
 wr32(code+0x74,0xe5850000u); /* STR r0,[r5] guest writes applet pointer */
 wr32(code+0x78,0xe59f3014u); /* LDR r3,[pc,#0x14] applet vtable at 0x94 */
 wr32(code+0x7c,0xe5803000u); /* STR r3,[r0] */
 wr32(code+0x80,0xe3a00000u); /* MOV r0,#0 success */
 wr32(code+0x84,0xe1a0e004u); /* MOV lr,r4 */
 wr32(code+0x88,0xe12fff1eu); /* BX lr */
 wr32(code+0x90,CNK_ARM_API_BASE+(6u<<12)+26u*4u);
 wr32(code+0x94,CNK_ARM_MOD_BASE+0xd0u);
 /* Zeebx upstream Shell::start_applet: actual guest vtable slot 2.
  * Handler stores CLSID and display pointer to its OWN applet object,
  * returning AEE TRUE, not through an invented host-side game loop. */
 wr32(code+0xd8,CNK_ARM_MOD_BASE+0xa0u);
 wr32(code+0xa0,0xe1a0400eu); /* MOV r4,lr */
 wr32(code+0xa4,0xe1a05000u); /* MOV r5,r0 */
 wr32(code+0xa8,0xe1a06001u); /* MOV r6,r1: original EVT_APP_START */
 wr32(code+0xac,0xe5937004u); /* LDR r7,[r3,#4] guest AEEAppStart cls */
 wr32(code+0xb0,0xe5857004u); /* STR r7,[r5,#4] guest applet memory */
 wr32(code+0xb4,0xe5937008u); /* LDR r7,[r3,#8] guest display ptr */
 wr32(code+0xb8,0xe5857008u); /* STR r7,[r5,#8] */
 wr32(code+0xbc,0xe3a00001u); /* MOV r0,#1, event handled */
 wr32(code+0xc0,0xe1a0e004u); /* MOV lr,r4 */
 wr32(code+0xc4,0xe12fff1eu); /* BX lr */
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome out={0};
 uint32_t calls=0,entry=0,applet=0,old=0;
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED && cpu.regs[0]==0u);
 assert(calls==1u && cpu.heap_cursor==40u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+8u,&old));
 assert(old==CNK_ARM_HEAP_BASE);
 assert(!cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry));
 assert(entry==CNK_ARM_MOD_BASE+0x60u);
 assert(cpu.regs[0]==CNK_ARM_HEAP_BASE);
 assert(cpu.regs[1]==CNK_ARM_OBJECT_BASE);
 assert(cpu.regs[2]==0x01081984u);
 assert(cpu.regs[3]==CNK_ARM_OBJECT_BASE+12u);
 assert(cpu.regs[14]==CNK_ARM_RETURN_MAGIC);
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 if(out.reason!=CNK_ARM_RETURNED || calls!=1u)
  fprintf(stderr,"stage2 status=%d pc=%08x opcode=%08x api=%u:%u calls=%u executed=%llu r0=%08x r3=%08x r5=%08x heap=%u\\n",
   (int)out.reason,out.pc,out.opcode,out.api_interface,out.api_slot,calls,
   (unsigned long long)out.executed,cpu.regs[0],cpu.regs[3],cpu.regs[5],cpu.heap_cursor);
 assert(out.reason==CNK_ARM_RETURNED && calls==1u);
 assert(cpu.heap_cursor==64u);
 assert(!cnk_zeebx_arm_applet_result(&cpu,&out,&applet));
 assert(applet==CNK_ARM_HEAP_BASE+40u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,applet,&old));
 assert(old==CNK_ARM_MOD_BASE+0xd0u);
 assert(cnk_zeebx_arm_applet_result(&cpu,&out,&applet)<0);
 /* Stage 3 really invokes original guest IApplet::HandleEvent. */
 uint32_t start_entry=0,start_struct=0,mem=0,display=0;
 assert(!cnk_zeebx_arm_prepare_start_event(
     &cpu,applet,0x01081984u,&start_entry,&start_struct));
 assert(start_entry==CNK_ARM_MOD_BASE+0xa0u);
 assert(start_struct==CNK_ARM_HEAP_BASE+64u);
 assert(cpu.regs[0]==applet && cpu.regs[1]==0u &&
        cpu.regs[2]==0u && cpu.regs[3]==start_struct);
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct,&mem)&&mem==0u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct+4u,&mem)&&mem==0x01081984u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct+8u,&display));
 assert(display==CNK_ARM_OBJECT_BASE+16u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,display,&mem) &&
        mem==CNK_ARM_VTABLE_BASE+5u*CNK_ARM_VTABLE_STRIDE);
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct+12u,&mem)&&mem==0u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct+16u,&mem)&&
        mem==(640u|(480u<<16)));
 assert(!cnk_zeebx_arm_read_u32(&cpu,start_struct+20u,&mem)&&mem==0u);
 assert(cpu.heap_cursor==88u&&cpu.object_cursor==80u);
 assert(cpu.display_objects==1u && cpu.applet_start_dispatched);
 assert(cnk_zeebx_arm_prepare_start_event(
     &cpu,applet,0x01081984u,&start_entry,&start_struct)<0);
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED && cpu.regs[0]==1u);
 assert(cpu.regs[6]==0u); /* event number was EVT_APP_START */
 assert(!cnk_zeebx_arm_read_u32(&cpu,applet+4u,&mem)&&mem==0x01081984u);
 assert(!cnk_zeebx_arm_read_u32(&cpu,applet+8u,&mem)&&mem==display);
 cnk_zeebx_arm_destroy(&cpu);

 /* End-to-end input path: parse a synthetic user-owned MIF, then
  * run BOTH guest functions and write an honest lifecycle report. */
 const char *modfile="test_zeebx_applet_guest.bin";
 const char *miffile="test_zeebx_applet_mif.bin";
 const char *logfile="test_zeebx_applet_report.txt";
 uint8_t manifest[128]={0};
 manifest[0]=0x11u;
 wr32(manifest+0x10,0x20u); /* offset of section table */
 wr32(manifest+0x14,1u);    /* one 20-byte applet record */
 wr32(manifest+0x20,0x40u); /* first section start */
 wr32(manifest+0x24,0x54u); /* end of 20-byte record */
 wr32(manifest+0x40,0x01081984u); /* genuine documented applet CLSID */
 FILE *file=fopen(modfile,"wb");
 assert(file);
 assert(fwrite(code,1,sizeof(code),file)==sizeof(code));
 assert(!fclose(file));
 file=fopen(miffile,"wb");
 assert(file);
 assert(fwrite(manifest,1,sizeof(manifest),file)==sizeof(manifest));
 assert(!fclose(file));
 assert(!cnk_zeebx_probe_original_applet(modfile,miffile,logfile,1000u,&out));
 assert(out.reason==CNK_ARM_RETURNED);
 char report[1600]={0};
 file=fopen(logfile,"rb");assert(file);
 size_t report_len=fread(report,1,sizeof(report)-1u,file);
 assert(!fclose(file));report[report_len]='\0';
 assert(strstr(report,"module_load_completed=YES"));
 assert(strstr(report,"applet_clsid=0x01081984"));
 assert(strstr(report,"applet_attempted=YES"));
 assert(strstr(report,"applet_created=YES"));
 assert(strstr(report,"applet_ptr=0x10000028"));
 assert(strstr(report,"start_event_attempted=YES"));
 assert(strstr(report,"start_event_completed=YES"));
 assert(strstr(report,"start_event_handled=YES"));
 assert(strstr(report,"start_event_reason=RETURNED"));
 assert(strstr(report,"start_event_entry=0x000100a0"));
 assert(strstr(report,"genuine_game_playable=NO"));
 /* A guest handler can RETURN but deliberately decline this event.
  * Diagnostics must not confuse a function return with a handled event. */
 wr32(code+0xbc,0xe3a00000u); /* MOV r0,#0; guest returns FALSE */
 file=fopen(modfile,"wb");assert(file);
 assert(fwrite(code,1,sizeof(code),file)==sizeof(code));
 assert(!fclose(file));
 assert(!cnk_zeebx_probe_original_applet(modfile,miffile,logfile,1000u,&out));
 memset(report,0,sizeof(report));
 file=fopen(logfile,"rb");assert(file);
 report_len=fread(report,1,sizeof(report)-1u,file);
 assert(!fclose(file));report[report_len]='\\0';
 assert(strstr(report,"start_event_completed=YES"));
 assert(strstr(report,"start_event_handled=NO"));
 assert(!remove(modfile));assert(!remove(miffile));assert(!remove(logfile));

 /* Corrupt guest module vtable pointer: it must NOT be called as ARM. */
 wr32(code+0x40,0xffffffffu);
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED);
 assert(cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry)<0);
 assert(!cpu.applet_pending);
 cnk_zeebx_arm_destroy(&cpu);

 /* A forged guest HandleEvent vtable MUST NOT start a fake game nor
  * consume heap bytes, change registers or create IDisplay. */
 wr32(code+0x40,CNK_ARM_MOD_BASE+0xc0u); /* restore valid module */
 wr32(code+0xd8,0xdeadbeefu); /* invalid guest applet method */
 assert(!cnk_zeebx_arm_init(&cpu,code,sizeof(code)));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(out.reason==CNK_ARM_RETURNED);
 assert(!cnk_zeebx_arm_prepare_applet(&cpu,0x01081984u,&entry));
 assert(!cnk_zeebx_arm_bootstrap(&cpu,1000u,&out,&calls));
 assert(!cnk_zeebx_arm_applet_result(&cpu,&out,&applet));
 const uint32_t saved_pc=cpu.regs[15],saved_heap=cpu.heap_cursor;
 const uint32_t saved_objs=cpu.object_cursor;
 uint32_t invalid_entry=0,invalid_start=0;
 assert(cnk_zeebx_arm_prepare_start_event(&cpu,applet,0x01081984u,
                                          &invalid_entry,&invalid_start)<0);
 assert(!invalid_entry && !invalid_start && !cpu.applet_start_dispatched);
 assert(cpu.regs[15]==saved_pc && cpu.heap_cursor==saved_heap);
 assert(cpu.object_cursor==saved_objs && !cpu.display_objects);
 /* The applet itself may be valid yet have no executable method; this
  * cannot be interpreted as a completed stage 3 or as playable. */
 cnk_zeebx_arm_destroy(&cpu);

 puts("PASS: Zeebx original module/create/start event, valid 24-byte AEEAppStart and invalid guest method preflight");
 return 0;
}
