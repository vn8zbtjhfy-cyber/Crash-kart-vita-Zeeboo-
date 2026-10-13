#include "vita_debug.h"
#include <psp2/io/fcntl.h>
#include <string.h>

#define CNK_DEBUG_FILE "ux0:data/cnk_vita_boot.log"
#define CNK_DEBUG_PREV1 "ux0:data/cnk_vita_boot.prev1.log"
#define CNK_DEBUG_PREV2 "ux0:data/cnk_vita_boot.prev2.log"

// Each stage is independently opened/written/closed so a psp2core crash does
// not discard buffered diagnostics. Errors must never crash the logger.
static void write_line(const char *s) {
 if (!s) return;
 SceUID fd = sceIoOpen(CNK_DEBUG_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
 if(fd<0)return;
 size_t n = strlen(s);
 if(n>512)n=512;
 if(n) sceIoWrite(fd,s,(SceSize)n);
 sceIoWrite(fd,"\n",1);
 sceIoClose(fd);
}
void vita_debug_reset(void){
 // Preserve logs from the previous two attempts, including hard crashes.
 // Old behavior truncated the only log on restart, hiding intermittent
 // C2-12828-1 crashes that occur only after reaching the real Vita menu.
 sceIoRemove(CNK_DEBUG_PREV2);
 sceIoRename(CNK_DEBUG_PREV1,CNK_DEBUG_PREV2);
 sceIoRename(CNK_DEBUG_FILE,CNK_DEBUG_PREV1);
 SceUID fd=sceIoOpen(CNK_DEBUG_FILE,SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);
 if(fd>=0)sceIoClose(fd);
}
void vita_debug_log(const char *line){write_line(line);}
void zeebx_vita_debug_log(const char *line){write_line(line);}
