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
 cnk_arm_outcome load_out={0},applet_out={0},signal_out={0};
 uint32_t module_calls=0,applet_calls=0,callbacks_delivered=0u;
 int callback_probe_rc=0,callback_probe_attempted=0;
 uint32_t modptr=0,apptr=0,guest_entry=0,clsid=0;
 int started=0,module_ok=0,applet_ok=0,prep=-1;
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
 /* Only at the original applet's verified guest return boundary.
  * Pending BREW HID notifications are deferred and executed as guest ARM,
  * not host-native function pointers. A failed callback is reported,
  * never silently treated as a playable successful launch. */
 if(applet_ok && cpu.signal_pending_count){
  callback_probe_attempted=1;
  callback_probe_rc=cnk_zeebx_arm_deliver_signals(
   &cpu,budget,&signal_out,&callbacks_delivered);
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
  "applet_created=%s\nsource_display_instances=%u\n"
  "source_filemgr_instances=%u\nguest_dbgprintf_calls=%u\n"
  "signal_callbacks_attempted=%s\nsignal_callbacks_delivered=%u\n"
  "signal_callbacks_remaining=%u\nsignal_dispatch_code=%d\n"
  "signal_dispatch_reason=%s\n"
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
  cpu.display_objects,cpu.filemgr_objects,cpu.debug_calls,
  callback_probe_attempted?"YES":"NO",callbacks_delivered,
  cpu.signal_pending_count,callback_probe_rc,
  callback_probe_attempted?reason_name(signal_out.reason):"NOT_RUN",
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
