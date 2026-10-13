#include "zeebx_bridge.h"
#include "vita_input.h"
#include "vita_video.h"
#include "vita_debug.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdint.h>
#include <stdio.h>

// A runtime crash on Vita is not a successful port. Persist boot stages for
// investigation in ux0:data/cnk_vita_boot.log without hiding fatal errors.
// The memory probe uses a supported VitaSDK syscall, never allocations.
static void vita_log_free_memory(const char *stage) {
 SceKernelFreeMemorySizeInfo info = {0};
 info.size = sizeof(info);
 int rc = sceKernelGetFreeMemorySize(&info);
 char message[192];
 if (rc == 0) {
   snprintf(message, sizeof(message),
     "C.memory.%s.user_kb=%d.cdram_kb=%d.phycont_kb=%d",
     stage, info.size_user / 1024, info.size_cdram / 1024,
     info.size_phycont / 1024);
 } else {
   snprintf(message,sizeof(message),
     "C.memory.%s.query_failed=%08x",stage,(unsigned)rc);
 }
 vita_debug_log(message);
}
static void vita_log_speed(uint32_t ticks,uint32_t drawn) {
 // Process-time and virtual clock are different measurements. Comparing
 // them tells us whether the Vita is compute-limited before blaming GL.
 unsigned long long process_ms =
   (unsigned long long)(sceKernelGetProcessTimeWide()/1000u);
 char message[192];
 snprintf(message,sizeof(message),
   "C.speed.ticks=%u.drawn=%u.process_ms=%llu.guest_ms=%u.gl_swaps=%u",
   (unsigned)ticks,(unsigned)drawn,process_ms,
   (unsigned)zeebx_vita_guest_clock_ms(),
   (unsigned)zeebx_vita_guest_gl_swaps());
 vita_debug_log(message);
}
int main(void) {
 vita_debug_reset();
 vita_debug_log("C.main.enter");
 vita_debug_log("C.build.vita_crash_diagnostics_v1");
 vita_log_free_memory("startup");
 vita_input_init();
 vita_debug_log("C.video.init.enter");
 if (vita_video_init()!=0) {
   vita_debug_log("C.video.init.failed");
   return 1;
 }
 vita_debug_log("C.video.init.ok");
 vita_log_free_memory("video_ready");

 const EmbeddedAsset m=embedded_asset_find("brew/mod/274214/cnk2.mod");
 const EmbeddedAsset v=embedded_asset_find("brew/mod/274214/data.vfs");
 const EmbeddedAsset f=embedded_asset_find("brew/mif/274214.mif");
 if(!m.bytes||!v.bytes||!f.bytes){
   vita_debug_log("C.assets.missing");
   vita_video_shutdown();
   return 2;
 }
 vita_debug_log("C.assets.found");
 vita_debug_log("C.zeebx.boot.enter");
 int boot_result=zeebx_vita_boot(embedded_asset_find);
 if(boot_result!=0){
   vita_debug_log("C.zeebx.boot.failed");
   vita_debug_log(zeebx_vita_last_error());
   vita_video_shutdown();
   return 3;
 }
 vita_debug_log("C.zeebx.boot.ok");
 vita_log_free_memory("after_boot");
 const uint8_t*rgba=0;
 uint32_t w=0,h=0,frames=0,drawn=0;
 for(;;){
   ZeebxVitaPad pad=vita_input_read();
   if(vita_input_should_exit())break;
   if(frames==0)vita_debug_log("C.zeebx.step.first.enter");
   // A long guest callback often yields a budget but no framebuffer yet.
   // Keep working immediately in that case, rather than adding 1ms of
   // artificial sleep on every ARM interpreter slice.
   const uint64_t instructions_before=zeebx_vita_guest_instructions();
   int result=zeebx_vita_step(&pad,&rgba,&w,&h);
   if(result<0){
     vita_debug_log("C.zeebx.step.failed");
     vita_debug_log(zeebx_vita_last_error());
     break;
   }
   if(result==1){
     if(drawn==0)vita_debug_log("C.video.draw.first.enter");
     const int video_error=vita_video_draw(rgba,w,h);
     if(video_error!=0){
       char message[96];
       snprintf(message,sizeof(message),"C.video.error.code=0x%08x",(unsigned)video_error);
       vita_debug_log(message);
       vita_debug_log("C.video.render.failed");
       break;
     }
     if(drawn++==0){
       vita_debug_log("C.video.draw.first.ok");
       vita_log_free_memory("first_frame");
     }
     // Memory sampling every 60 presented frames. Sparse diagnostics help
     // identify an out-of-memory crash after the original game's menu.
     if(drawn%60==0)vita_log_free_memory("frame_checkpoint");
   }else{
     // Sleep only when Zeebx is genuinely idle (no guest CPU progress).
     // This also lets us poll the input controller more frequently during
     // interpreter-heavy game screens and race transitions.
     if(zeebx_vita_guest_instructions()==instructions_before)
       sceKernelDelayThread(1000);
   }
   if((++frames%180)==0)vita_log_speed(frames,drawn);
 }
 vita_debug_log("C.shutdown");
 zeebx_vita_shutdown();
 vita_video_shutdown();
 return 0;
}
