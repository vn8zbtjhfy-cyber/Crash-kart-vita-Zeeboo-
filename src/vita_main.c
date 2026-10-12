/* Main entry for the native PS Vita game under development.
 * This starts the actual source-data racing runtime, not the old PNG viewer.
 * It is NOT wired to produce a VPK or claimed as complete until the original
 * mechanics, all assets and console testing are validated.
 * The former texture viewer remains in legacy/vita_png_viewer_original.c. */
#include "cnk_vita_native_app.h"
#include "cnk_vita_source_path.h"
#include <psp2/kernel/processmgr.h>

int main(void){
    const char *archive=cnk_vita_source_find();
    /* Report a missing FUFS archive using the existing diagnostic logger.
     * Never silently substitute a non-original or empty data file. */
    const int result=cnk_vita_native_run(archive?archive:"ux0:data/cnk3d/data.vfs");
    const int status=result==0?0:1;
    sceKernelExitProcess(status);
    return status;
}
