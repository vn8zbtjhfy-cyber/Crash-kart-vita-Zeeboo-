#include "zeebx_bridge.h"
#include "vita_input.h"
#include "vita_video.h"
#include "vita_debug.h"
#include <psp2/kernel/threadmgr.h>
#include <stdint.h>

// A runtime crash on Vita is not a successful port. Persist boot stages for
// investigation in ux0:data/cnk_vita_boot.log without hiding fatal errors.
int main(void) {
 vita_debug_reset();
 vita_debug_log("C.main.enter");
 vita_input_init();
 vita_debug_log("C.video.init.enter");
 if (vita_video_init()!=0) {
   vita_debug_log("C.video.init.failed");
   return 1;
 }
 vita_debug_log("C.video.init.ok");

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
     vita_video_draw(rgba,w,h);
     if(drawn++==0)vita_debug_log("C.video.draw.first.ok");
   }else{
     // Sleep only when Zeebx is genuinely idle (no guest CPU progress).
     // This also lets us poll the input controller more frequently during
     // interpreter-heavy game screens and race transitions.
     if(zeebx_vita_guest_instructions()==instructions_before)
       sceKernelDelayThread(1000);
   }
   if((++frames%180)==0)vita_debug_log("C.frame.heartbeat.180");
 }
 vita_debug_log("C.shutdown");
 zeebx_vita_shutdown();
 vita_video_shutdown();
 return 0;
}
