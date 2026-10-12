/* Main entry for the native PS Vita game under development.
 * This starts the actual source-data racing runtime, not the old PNG viewer.
 * It is NOT wired to produce a VPK or claimed as complete until the original
 * mechanics, all assets and console testing are validated.
 * The former texture viewer remains in legacy/vita_png_viewer_original.c. */
#include "cnk_vita_native_app.h"
#include "cnk_vita_source_path.h"
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
    /* Optional original Zeebx graphics probe now runs AFTER vglInit()
     * inside cnk_vita_native_run. Running it here, before the GPU is
     * initialized, cannot safely execute original GL calls.
     */
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
