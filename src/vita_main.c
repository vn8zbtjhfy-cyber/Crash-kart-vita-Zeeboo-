/* Main entry for the native PS Vita game under development.
 * This starts the actual source-data racing runtime, not the old PNG viewer.
 * It is NOT wired to produce a VPK or claimed as complete until the original
 * mechanics, all assets and console testing are validated.
 * The former texture viewer remains in legacy/vita_png_viewer_original.c. */
#include "cnk_vita_native_app.h"
#include "cnk_vita_source_path.h"
#include "cnk_zeebx_probe.h"
#include <stdio.h>
#include <psp2/kernel/processmgr.h>

int main(void){
    /* An opt-in diagnostic path executes the original Zeebo ARM start
     * through our Zeebx-derived INTERPRETER, never as host/native code.
     * It is OFF by default, fully isolated, and NEVER substitutes the
     * already implemented native race runtime below.
     *
     * User-owned files on their PS Vita only:
     *  ux0:data/cnk3d/cnk2.mod
     *  ux0:data/cnk3d/274214.mif
     *  ux0:data/cnk3d/zeebx_probe.flag   (empty marker enables probe)
     * Report: ux0:data/cnk3d/zeebx_probe.txt
     */
    FILE *zeebx_flag=fopen("ux0:data/cnk3d/zeebx_probe.flag","rb");
    if(zeebx_flag){
        fclose(zeebx_flag);
        cnk_arm_outcome probe={0};
        int probe_rc=cnk_zeebx_probe_original_applet(
            "ux0:data/cnk3d/cnk2.mod",
            "ux0:data/cnk3d/274214.mif",
            "ux0:data/cnk3d/zeebx_probe.txt",200000u,&probe);
        fprintf(stderr,"CNK: Zeebx guest ARM probe status %d, "
                       "steps=%llu stop=%u\\n",
                probe_rc,(unsigned long long)probe.executed,
                (unsigned)probe.reason);
    }
    const char *archive=cnk_vita_source_find();
    /* Report a missing FUFS archive using the existing diagnostic logger.
     * Never silently substitute a non-original or empty data file. */
    const int result=cnk_vita_native_run(archive?archive:"ux0:data/cnk3d/data.vfs");
    const int status=result==0?0:1;
    sceKernelExitProcess(status);
    return status;
}
