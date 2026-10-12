/* Main entry for the native PS Vita game under development.
 * This starts the actual source-data racing runtime, not the old PNG viewer.
 * It is NOT wired to produce a VPK or claimed as complete until the original
 * mechanics, all assets and console testing are validated.
 * The former texture viewer remains in legacy/vita_png_viewer_original.c. */
#include "cnk_vita_native_app.h"
#include "cnk_vita_source_path.h"
#include "cnk_zeebx_probe.h"
#include "cnk_vita_controls.h"
#include <stdio.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/stat.h>

/* Vita homebrew must create its own writable save/data folder on startup.
 * Never depend on the user creating ux0:data/cnk3d manually.
 * This cannot manufacture the proprietary Zeebo FUFS data.vfs: it only
 * prepares the user-owned directory and creates an honest launch report. */
static void cnk_launch_report(const char *mode,const char *phase,int result){
    FILE *out=fopen("ux0:data/cnk3d/launch_status.txt",mode);
    if(!out)out=fopen("ux0:data/cnk3d_launch_status.txt",mode);
    if(!out)return;
    fprintf(out,"%s code=%d\n",phase,result);
    fclose(out);
}

int main(void){
    (void)sceIoMkdir("ux0:data",0777);
    (void)sceIoMkdir("ux0:data/cnk3d",0777);
    cnk_launch_report("wb","START_AND_AUTOCREATE_DIRECTORY",0);
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
        /* One real controller snapshot enters guest memory; the probe is
         * still bounded, and live per-frame guest input needs further work.
         * No sample means neutral, never a held phantom button. */
        cnk_zeebx_pad guest_pad={0};
        for(unsigned i=0;i<CNK_ZEEBX_PAD_AXES;i++)guest_pad.axes[i]=128u;
        if(cnk_vita_controls_init()==0)
            (void)cnk_vita_controls_read_zeebx(&guest_pad);
        int probe_rc=cnk_zeebx_probe_original_applet_with_pad(
            "ux0:data/cnk3d/cnk2.mod",
            "ux0:data/cnk3d/274214.mif",
            "ux0:data/cnk3d/zeebx_probe.txt",200000u,&guest_pad,&probe);
        fprintf(stderr,"CNK: Zeebx guest ARM probe status %d, "
                       "steps=%llu stop=%u\\n",
                probe_rc,(unsigned long long)probe.executed,
                (unsigned)probe.reason);
    }
    const char *archive=cnk_vita_source_find();
    /* An absent or invalid FUFS is not an app crash: report the exact
     * missing prerequisite before the native runtime's diagnostic exit. */
    if(!archive)
        cnk_launch_report("ab","ORIGINAL_DATA_VFS_MISSING_OR_INVALID",-1);
    else
        cnk_launch_report("ab","ORIGINAL_DATA_VFS_FOUND",0);
    const int result=cnk_vita_native_run(archive?archive:"ux0:data/cnk3d/data.vfs");
    cnk_launch_report("ab","RUNTIME_RETURN_CODE",result);
    const int status=result==0?0:1;
    sceKernelExitProcess(status);
    return status;
}
