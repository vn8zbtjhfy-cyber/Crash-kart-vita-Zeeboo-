#include "zeebx_bridge.h"
#include "vita_input.h"
#include "vita_video.h"
#include <psp2/kernel/threadmgr.h>
#include <stdint.h>
int main(void){
 vita_input_init();if(vita_video_init()!=0)return 1;
 const EmbeddedAsset m=embedded_asset_find("brew/mod/274214/cnk2.mod");
 const EmbeddedAsset v=embedded_asset_find("brew/mod/274214/data.vfs");
 const EmbeddedAsset f=embedded_asset_find("brew/mif/274214.mif");
 if(!m.bytes||!v.bytes||!f.bytes||zeebx_vita_boot(embedded_asset_find)){
  vita_video_shutdown();return 2;
 }
 const uint8_t*rgba=0;uint32_t w=0,h=0;
 while(1){ZeebxVitaPad pad=vita_input_read();if(vita_input_should_exit())break;
  int result=zeebx_vita_step(&pad,&rgba,&w,&h);
  if(result<0)break;
  if(result==1)vita_video_draw(rgba,w,h);else sceKernelDelayThread(1000);
 }
 zeebx_vita_shutdown();vita_video_shutdown();return 0;
}
