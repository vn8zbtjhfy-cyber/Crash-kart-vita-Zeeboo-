/* Original Zeebx module/APPMAN execution probe, GPL-2.0-or-later.
 * Reads private .mod/.mif from user's own filesystem only.
 * Every unimplemented BREW API, CPU instruction and memory fault
 * is an explicit stop. This NEVER replaces the native racing engine.
 */
#include "cnk_zeebx_probe.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int private_file(const char *path,size_t limit,uint8_t **bytes,size_t *size){
 if(!path||!bytes||!size)return -1;
 *bytes=NULL;*size=0;
 FILE *f=fopen(path,"rb");
 if(!f)return -2;
 if(fseek(f,0,SEEK_END)){fclose(f);return -3;}
 long length=ftell(f);
 if(length<=0||(unsigned long)length>limit||
    fseek(f,0,SEEK_SET)){fclose(f);return -3;}
 uint8_t *p=(uint8_t*)malloc((size_t)length);
 if(!p){fclose(f);return -4;}
 if(fread(p,1,(size_t)length,f)!=(size_t)length){
  fclose(f);free(p);return -5;
 }
 if(fclose(f)){free(p);return -6;}
 *bytes=p;*size=(size_t)length;return 0;
}
static const char *reason_name(cnk_arm_reason r){
 switch(r){
 case CNK_ARM_API:return "BREW_API";
 case CNK_ARM_RETURNED:return "RETURNED";
 case CNK_ARM_FAULT:return "MEMORY_FAULT";
 case CNK_ARM_UNSUPPORTED:return "UNSUPPORTED_OPCODE";
 default:return "INSTRUCTION_BUDGET";
 }
}
static int probe_internal(const char *mod_path,const char *mif_path,
                          const char *report_path,uint32_t budget,
                          const cnk_zeebx_pad *pad,const cnk_zeebx_gl_host *host,
                          void *host_user,cnk_arm_outcome *result){
 uint8_t *mod=NULL,*mif=NULL;
 size_t mod_bytes=0,mif_bytes=0;
 cnk_brew_mif_info manifest={0};
 cnk_zeebx_arm cpu={0};
 cnk_arm_outcome load_out={0},applet_out={0},signal_out={0},start_out={0},timer_out={0};
 uint32_t module_calls=0,applet_calls=0,start_calls=0,callbacks_delivered=0u;
 uint32_t timer_callbacks=0u;
 int callback_probe_rc=0,callback_probe_attempted=0;
 int timer_probe_attempted=0,timer_probe_rc=0;
 uint32_t modptr=0,apptr=0,guest_entry=0,clsid=0;
 uint32_t start_entry=0,start_ptr=0;
 int started=0,module_ok=0,applet_ok=0,prep=-1;
 int start_prep=-1,start_attempted=0,start_completed=0,start_handled=0;
 if(!result)return -1;
 memset(result,0,sizeof(*result));
 if(!mod_path||!report_path||!budget||budget>10000000u)return -1;
 int rc=private_file(mod_path,16u*1024u*1024u,&mod,&mod_bytes);
 if(rc)return -2;
 if(mif_path){
  rc=private_file(mif_path,1024u*1024u,&mif,&mif_bytes);
  if(rc){free(mod);return -3;}
  if(cnk_brew_mif_inspect(mif,mif_bytes,&manifest)||
     !manifest.main_applet){
   free(mod);free(mif);return -4;
  }
  clsid=manifest.main_applet;
  free(mif);
 }
 rc=cnk_zeebx_arm_init(&cpu,mod,mod_bytes);
 free(mod);
 if(rc)return -5;
 /* One private original game root, based on its own .mod directory.
  * Zeebx guest names never reach arbitrary Vita or PC host paths. */
 {
  const char *slash=strrchr(mod_path,'/');
  char root[CNK_ZEEBX_FILE_ROOT_CHARS]={0};
  size_t n=slash?(size_t)(slash-mod_path):1u;
  if(!slash){root[0]='.';}
  else if(n>0u&&n<sizeof(root))memcpy(root,mod_path,n);
  else {cnk_zeebx_arm_destroy(&cpu);return -5;}
  if(cnk_zeebx_arm_set_file_root(&cpu,root)){
   cnk_zeebx_arm_destroy(&cpu);return -5;
  }
 }
 if(pad && cnk_zeebx_arm_set_pad(&cpu,pad)){
  cnk_zeebx_arm_destroy(&cpu);return -5;
 }
 /* Only GPU-initialized Vita callers supply real GPU callbacks. */
 if(host && cnk_zeebx_arm_set_gl_host(&cpu,host,host_user)){
  cnk_zeebx_arm_destroy(&cpu);return -5;
 }
 rc=cnk_zeebx_arm_bootstrap(&cpu,budget,&load_out,&module_calls);
 if(rc){cnk_zeebx_arm_destroy(&cpu);return -6;}
 module_ok=load_out.reason==CNK_ARM_RETURNED && cpu.regs[0]==0u;
 if(!cnk_zeebx_arm_read_u32(&cpu,CNK_ARM_OBJECT_BASE+8u,&modptr)){
  module_ok=module_ok&&modptr>=CNK_ARM_HEAP_BASE&&
     (uint64_t)modptr+4u<=
       (uint64_t)CNK_ARM_HEAP_BASE+cpu.heap_cursor;
 }else{
  modptr=0;module_ok=0;
 }
 *result=load_out;
 if(clsid&&module_ok){
  started=1;
  prep=cnk_zeebx_arm_prepare_applet(&cpu,clsid,&guest_entry);
  if(!prep){
   rc=cnk_zeebx_arm_bootstrap(&cpu,budget,&applet_out,&applet_calls);
   if(rc){cnk_zeebx_arm_destroy(&cpu);return -7;}
   applet_ok=!cnk_zeebx_arm_applet_result(&cpu,&applet_out,&apptr);
   *result=applet_out;
  }
 }
 /* Stage 3 of upstream Zeebx Machine::start_applet. The genuine guest
  * IApplet HandleEvent(slot 2) receives EVT_APP_START=0 and its 24-byte
  * AEEAppStart. Only a RETURNED result completes this stage. Graphics,
  * audio and BREW APIs not yet implemented remain explicit traps. */
 if(applet_ok){
  start_prep=cnk_zeebx_arm_prepare_start_event(
   &cpu,apptr,clsid,&start_entry,&start_ptr);
  if(!start_prep){
   start_attempted=1;
   rc=cnk_zeebx_arm_bootstrap(&cpu,budget,&start_out,&start_calls);
   if(rc){cnk_zeebx_arm_destroy(&cpu);return -10;}
   start_completed=start_out.reason==CNK_ARM_RETURNED &&
                   cpu.regs[15]==CNK_ARM_RETURN_MAGIC;
   start_handled=start_completed && cpu.regs[0]!=0u;
   *result=start_out;
  }
 }
 /* Only at a verified, completed original guest return boundary.
  * Never dispatch signals inside an unresolved START guest API trap. */
 if(start_completed && cpu.signal_pending_count){
  callback_probe_attempted=1;
  callback_probe_rc=cnk_zeebx_arm_deliver_signals(
   &cpu,budget,&signal_out,&callbacks_delivered);
 }
 /* Immediate timers scheduled by EVT_APP_START must be dispatched at
  * the return boundary, not during an ARM BREW trampoline.
  * Future frame timers require an actual clock-driven Vita guest loop. */
 if(start_completed &&
    (!callback_probe_attempted ||
     (!callback_probe_rc && signal_out.reason==CNK_ARM_RETURNED)) &&
    cpu.timer_count){
  timer_probe_attempted=1;
  timer_probe_rc=cnk_zeebx_arm_deliver_timers(
   &cpu,budget,&timer_out,&timer_callbacks);
  if(timer_probe_rc || (timer_out.reason!=CNK_ARM_RETURNED &&
                        timer_out.reason!=CNK_ARM_BUDGET))
   *result=timer_out;
 }
 FILE *report=fopen(report_path,"wb");
 if(!report){cnk_zeebx_arm_destroy(&cpu);return -8;}
 int length=fprintf(report,
  "original_mod_size=%lu\nreason=%s\nsteps=%llu\npc=0x%08x\n"
  "api_interface=%u\napi_slot=%u\nopcode=0x%08x\naddress=0x%08x\n"
  "bootstrap_apis_handled=%u\nmodule_ptr=0x%08x\n"
  "module_load_completed=%s\n"
  "applet_clsid=0x%08x\napplet_attempted=%s\napplet_prepare_code=%d\n"
  "applet_entry=0x%08x\napplet_reason=%s\napplet_steps=%llu\n"
  "applet_api_interface=%u\napplet_api_slot=%u\n"
  "applet_apis_handled=%u\napplet_ptr=0x%08x\n"
  "applet_created=%s\nstart_event_attempted=%s\n"
  "start_event_prepare_code=%d\nstart_event_entry=0x%08x\n"
  "start_event_data=0x%08x\nstart_event_reason=%s\n"
  "start_event_api_interface=%u\nstart_event_api_slot=%u\n"
  "start_event_apis_handled=%u\nstart_event_completed=%s\n"
  "start_event_handled=%s\n"
  "source_display_instances=%u\n"
  "source_filemgr_instances=%u\nguest_dbgprintf_calls=%u\n"
  "zeebx_guest_files_open=%u\nzeebx_file_reads=%u\n"
  "zeebx_file_bytes_read=%u\nzeebx_file_path_denials=%u\n"
  "signal_callbacks_attempted=%s\nsignal_callbacks_delivered=%u\n"
  "signal_callbacks_remaining=%u\nsignal_dispatch_code=%d\n"
  "signal_dispatch_reason=%s\n"
  "guest_timer_callbacks_attempted=%s\n"
  "guest_timer_callbacks_delivered=%u\n"
  "guest_timers_remaining=%u\n"
  "guest_timer_dispatch_code=%d\n"
  "guest_timer_dispatch_reason=%s\n"
  "gpu_host_bound=%s\nguest_gl_clear_calls=%u\n"
  "guest_gl_viewports=%u\nguest_gl_presented_frames=%u\n"
  "genuine_game_playable=NO\n",
  (unsigned long)mod_bytes,reason_name(load_out.reason),
  (unsigned long long)load_out.executed,load_out.pc,
  load_out.api_interface,load_out.api_slot,load_out.opcode,
  load_out.address,module_calls,modptr,module_ok?"YES":"NO",
  clsid,started?"YES":"NO",prep,guest_entry,
  (started&&!prep)?reason_name(applet_out.reason):"NOT_RUN",
  (unsigned long long)applet_out.executed,applet_out.api_interface,
  applet_out.api_slot,applet_calls,apptr,applet_ok?"YES":"NO",
  start_attempted?"YES":"NO",start_prep,start_entry,start_ptr,
  start_attempted?reason_name(start_out.reason):"NOT_RUN",
  start_out.api_interface,start_out.api_slot,start_calls,
  start_completed?"YES":"NO",start_handled?"YES":"NO",
  cpu.display_objects,cpu.filemgr_objects,cpu.debug_calls,
  cpu.file_open_count,cpu.file_read_calls,cpu.file_bytes_read,
  cpu.file_path_denials,
  callback_probe_attempted?"YES":"NO",callbacks_delivered,
  cpu.signal_pending_count,callback_probe_rc,
  callback_probe_attempted?reason_name(signal_out.reason):"NOT_RUN",
  timer_probe_attempted?"YES":"NO",timer_callbacks,cpu.timer_count,
  timer_probe_rc,
  timer_probe_attempted?reason_name(timer_out.reason):"NOT_RUN",
  host?"YES":"NO",cpu.gl_clear_calls,cpu.gl_viewport_calls,
  cpu.gl_presents);
 int closed=fclose(report);
 cnk_zeebx_arm_destroy(&cpu);
 return (length<0||closed)?-9:0;
}
int cnk_zeebx_probe_original(const char *mod_path,const char *report_path,
                             uint32_t budget,cnk_arm_outcome *result){
 return probe_internal(mod_path,NULL,report_path,budget,NULL,NULL,NULL,result);
}
int cnk_zeebx_probe_original_applet(const char *mod_path,const char *mif_path,
                                    const char *report_path,uint32_t budget,
                                    cnk_arm_outcome *result){
 if(!mif_path)return -1;
 return probe_internal(mod_path,mif_path,report_path,budget,NULL,NULL,NULL,result);
}

int cnk_zeebx_probe_original_applet_with_pad(const char *mod_path,
                                              const char *mif_path,
                                              const char *report_path,
                                              uint32_t budget,
                                              const cnk_zeebx_pad *pad,
                                              cnk_arm_outcome *result){
 if(!mif_path || !pad)return -1;
 return probe_internal(mod_path,mif_path,report_path,budget,pad,NULL,NULL,result);
}

int cnk_zeebx_probe_original_applet_with_gl(
 const char *mod_path,const char *mif_path,const char *report_path,
 uint32_t budget,const cnk_zeebx_pad *pad,const cnk_zeebx_gl_host *host,
 void *host_user,cnk_arm_outcome *result){
 if(!mod_path||!mif_path||!pad||!host)return -1;
 return probe_internal(mod_path,mif_path,report_path,budget,
                       pad,host,host_user,result);
}
