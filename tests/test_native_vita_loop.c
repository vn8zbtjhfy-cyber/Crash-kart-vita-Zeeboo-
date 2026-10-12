#define _POSIX_C_SOURCE 200809L
/* Host control/GPU recorder used ONLY to exercise the Vita application
 * wiring. It is not a PSVita runtime or proof of performance on hardware. */
#include "cnk_vita_native_app.h"
#include "cnk_game.h"
#include "cnk_png.h"
#include "cnk_track_catalog.h"
#include "vitaGL.h"
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/stat.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

static int controller_reads=0,swaps=0,vblank_waits=0,inits=0,shutdowns=0,overlay_quads=0;
static unsigned save_directory_attempts=0;
int sceIoMkdir(const char *dir,unsigned int mode){
 assert(dir && strcmp(dir,"ux0:data")==0);
 assert(mode==0777u);
 save_directory_attempts++;
 /* An existing directory is not a reason to abort normal gameplay. */
 return -1;
}
extern int cnk_host_audio_opens,cnk_host_audio_closes,cnk_host_audio_outputs,cnk_host_audio_signal_blocks;
extern int cnk_host_audio_fail_at;
static int fail_vblank_on=0,inject_gl_error_on_vblank=0;
static uint64_t clocks=1000000;
static const uint32_t playback[]={
 0,SCE_CTRL_RIGHT,0,SCE_CTRL_CROSS,SCE_CTRL_CROSS,0,
 SCE_CTRL_START,0,SCE_CTRL_START,0,SCE_CTRL_START|SCE_CTRL_SELECT
 };
/* Allow the worker enough host time to report its deliberately injected
 * error under loaded CI machines; the ordinary 11-frame playback can exit
 * before a 10.7-ms SceAudioOut buffer is submitted. */
/* Source mesh gallery: R changes LOD while the frontend remains on SELECT. */
static const uint32_t lang_cycle_playback[]={
 0,SCE_CTRL_SELECT,SCE_CTRL_SELECT,0,SCE_CTRL_SELECT,0,
 SCE_CTRL_START|SCE_CTRL_SELECT
};
static const uint32_t kart_plain_playback[]={0,0,0,SCE_CTRL_START|SCE_CTRL_SELECT};
static const uint32_t kart_toggle_playback[]={0,SCE_CTRL_RTRIGGER,0,SCE_CTRL_START|SCE_CTRL_SELECT};
static const uint32_t gallery_orbit_playback[]={0,SCE_CTRL_LTRIGGER,0,SCE_CTRL_START|SCE_CTRL_SELECT};
static const uint32_t audio_failure_playback[96]={
 [95]=SCE_CTRL_START|SCE_CTRL_SELECT
};
static const uint32_t switch_playback[]={
 0,SCE_CTRL_RIGHT,0,SCE_CTRL_CROSS,0,
 SCE_CTRL_START,0,SCE_CTRL_CIRCLE,0,
 SCE_CTRL_RIGHT,0,SCE_CTRL_CROSS,0,
 SCE_CTRL_START|SCE_CTRL_SELECT
};
static const uint32_t *active_playback=playback;
static size_t active_playback_len=sizeof(playback)/sizeof(playback[0]);
int sceCtrlSetSamplingMode(int mode){return mode==SCE_CTRL_MODE_ANALOG?0:-1;}
int sceCtrlPeekBufferPositive(int port,SceCtrlData *pad,int count){
 if(port||count!=1||!pad)return -1;
 memset(pad,0,sizeof(*pad));
 pad->lx=128;pad->ly=128;
 unsigned index=(unsigned)controller_reads;
 if(index>=active_playback_len)return -1;
 pad->buttons=active_playback[index];controller_reads++;
 return 1;
}
uint64_t sceKernelGetProcessTimeWide(void){
 clocks+=16667u;
 return clocks;
}
void vglInit(unsigned int memory){assert(memory>0);inits++;}
void vglSwapBuffers(GLboolean vsync){
 assert(vsync==GL_FALSE);
 assert(cnk_test_gl.matrix_stack_depth==0);
 swaps++;
}
int sceDisplayWaitVblankStart(void){
 /* Exactly one presentation must precede each display sync. */
 assert(swaps==vblank_waits+1);vblank_waits++;
 /* On the next frame the Vita main loop must detect an injected GPU error
  * and release all resources without presenting the damaged frame. */
 if(inject_gl_error_on_vblank && vblank_waits==inject_gl_error_on_vblank)
   cnk_test_gl.queued_error=GL_OUT_OF_MEMORY;
 /* Host timing: give the independent audio thread time to submit buffers. */
 struct timespec audio_slice={0,1000000};nanosleep(&audio_slice,NULL);
 return (fail_vblank_on && vblank_waits==fail_vblank_on)?-1:0;
}
/* The public vitaGL API does not provide a teardown call. */
void vglEnd(void){shutdowns++;assert(!"unsupported vitaGL teardown called");}
void glBegin(GLenum mode){assert(mode==GL_QUADS);overlay_quads++;}
void glEnd(void){}
void glVertex2f(float x,float y){assert(isfinite(x)&&isfinite(y));}
void glLoadIdentity(void){}
void glVertex3f(float x,float y,float z){
 assert(isfinite(x)&&isfinite(y)&&isfinite(z));
}
void glOrtho(double left,double right,double bottom,double top,double near_v,double far_v){
 assert(left==0&&right==960&&bottom==544&&top==0&&near_v==-1&&far_v==1);
}
void glClearColor(float r,float g,float b,float a){assert(isfinite(r)&&isfinite(g)&&isfinite(b)&&isfinite(a));}
void glClear(GLenum mask){assert((mask&(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT))==(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));}
int main(int argc,char **argv){
 assert(argc==2);
 cnk_test_gl_reset(UINT32_MAX);
 /* AVANCE 107: a missing source archive must report a concrete startup
  * failure before GPU init, not fall through to a black-screen race. */
 assert(cnk_vita_native_run("/tmp/cnk107_missing_source_archive.vfs")==-1);
 assert(inits==0 && shutdowns==0 && swaps==0);
 int rc=cnk_vita_native_run(argv[1]);
 assert(rc==0);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(vblank_waits==swaps);
 assert(swaps==(int)(sizeof(playback)/sizeof(playback[0])-1));
 assert(controller_reads==(int)(sizeof(playback)/sizeof(playback[0])));
 assert(cnk_test_gl.draw_calls>0); /* real geometry through native draw bridge */
 assert(overlay_quads>=swaps);
 assert(cnk_test_gl.matrix_stack_depth==0);
 assert(cnk_test_gl.invalid==0);
 /* Change original LANG live in the SELECT menu without advancing track
  * or starting a race. Host controls and GL are simulated, not Vita QA. */
 active_playback=lang_cycle_playback;
 active_playback_len=sizeof(lang_cycle_playback)/sizeof(lang_cycle_playback[0]);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(swaps==(int)(active_playback_len-1) && inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.invalid==0 && cnk_test_gl.matrix_stack_depth==0);
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 /* A failed display sync must stop the loop and still release Vita GPU
  * and ROM state; this is an error test, not a hardware timing claim. */
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);fail_vblank_on=3;
 assert(cnk_vita_native_run(argv[1])==-13);
 assert(vblank_waits==3 && swaps==3 && inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.invalid==0 && cnk_test_gl.matrix_stack_depth==0);
 fail_vblank_on=0;
 /* Regression: errors can also be reported asynchronously by the GPU even
  * if the track/frame draw bridge returned zero. Stop before the next swap. */
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);inject_gl_error_on_vblank=2;
 assert(cnk_vita_native_run(argv[1])==-48);
 assert(vblank_waits==2 && swaps==2 && inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(controller_reads==3); /* third frame sampled but never presented */
 assert(cnk_test_gl.invalid==0 && cnk_test_gl.matrix_stack_depth==0);
 inject_gl_error_on_vblank=0;
 /* Test the complete production run with ONE strictly labeled synthetic
  * texture mapping. This map exists only temporarily inside the test and
  * is never packaged as an original Zeebo association. */
 fufs_file vfs={0};cnk_png_catalog png={0};cnk_track_catalog tracks={0};cnk_game game={0};
 assert(fufs_open(argv[1],&vfs)==0);
 assert(cnk_png_catalog_build(&vfs,&png)==0 && png.count==317);
 assert(cnk_track_catalog_build(&vfs,&tracks)==0);
 const cnk_track_entry *track=cnk_track_race_at(&tracks,1);
 assert(track!=NULL);
 assert(cnk_game_load(&game,&vfs,track->vfs_index,3,2)==0);
 uint32_t slot=UINT32_MAX;
 for(uint32_t i=0;i<game.render.batch_count;i++){
  uint32_t candidate=game.render.batches[i].texture;
  if(candidate<game.scene.texture_count && game.scene.texture_names[candidate][0]){
   slot=candidate;break;
  }
 }
 assert(slot!=UINT32_MAX);
 char path[]="/tmp/cnk_vita_native_texture_XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);
 FILE *fp=fdopen(fd,"wb");assert(fp);
 assert(fprintf(fp,"# SYNTHETIC HOST INTEGRATION INPUT, NEVER A REAL MAPPING\n"
   "%u\t%08x\t%u\t%s\t%u\t%08x\n",
   track->vfs_index,track->resource_id,slot,game.scene.texture_names[slot],
   png.items[0].vfs_index,png.items[0].vfs_id)>0);
 assert(fclose(fp)==0);
 cnk_game_unload(&game);
 fufs_close(&vfs);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_map(argv[1],path)==0);
 assert(cnk_test_gl.textured_calls>0);
 assert(cnk_test_gl.handles_created==1);
 assert(cnk_test_gl.handles_deleted==1);
 assert(cnk_test_gl.invalid==0);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(vblank_waits==swaps);
 /* One more integration: scene change must DELETE all GPU textures of the
  * preceding circuit before allocating any for the next. */
 fufs_file vfs2={0};cnk_game second={0};
 assert(fufs_open(argv[1],&vfs2)==0);
 const cnk_track_entry *track2=cnk_track_race_at(&tracks,2);
 assert(track2!=NULL);
 assert(cnk_game_load(&second,&vfs2,track2->vfs_index,3,2)==0);
 uint32_t slot2=UINT32_MAX;
 for(uint32_t i=0;i<second.render.batch_count;i++){
  uint32_t c=second.render.batches[i].texture;
  if(c<second.scene.texture_count && second.scene.texture_names[c][0]){
   slot2=c;break;
  }
 }
 assert(slot2!=UINT32_MAX);
 fp=fopen(path,"ab");assert(fp);
 assert(fprintf(fp,"%u\t%08x\t%u\t%s\t%u\t%08x\n",
   track2->vfs_index,track2->resource_id,slot2,second.scene.texture_names[slot2],
   png.items[0].vfs_index,png.items[0].vfs_id)>0);
 assert(fclose(fp)==0);
 cnk_game_unload(&second);fufs_close(&vfs2);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=switch_playback;
 active_playback_len=sizeof(switch_playback)/sizeof(switch_playback[0]);
 assert(cnk_vita_native_run_with_map(argv[1],path)==0);
 assert(cnk_test_gl.handles_created==2);
 assert(cnk_test_gl.handles_deleted==2);
 assert(cnk_test_gl.textured_calls>0);
 assert(cnk_test_gl.invalid==0);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(vblank_waits==swaps);
 /* Actual native application additionally attaches a user-selected ORIGINAL
  * 3DOB resource to a racer, without claiming this is the correct character. */
 fufs_file mvfs={0};assert(!fufs_open(argv[1],&mvfs));
 unsigned selected=UINT32_MAX;
 for(uint32_t j=0;j<mvfs.count;j++){
   unsigned char *bytes=NULL;size_t length=0;
   assert(!fufs_extract(&mvfs,j,&bytes,&length));
   int is_model=length>=12&&!memcmp(bytes,"PIFF",4)&&!memcmp(bytes+8,"3DOB",4);
   free(bytes);
   if(is_model){selected=j;break;}
 }
 assert(selected!=UINT32_MAX);
 char modelpath[]="/tmp/cnk_vita_model_XXXXXX";
 int mfd=mkstemp(modelpath);assert(mfd>=0);
 FILE *mf=fdopen(mfd,"wb");assert(mf);
 assert(fprintf(mf,"# HOST TEST ONLY (not character identity)\n"
    "0\t%u\t%08x\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n",
    selected,mvfs.entries[selected].id)>0);
 assert(!fclose(mf));fufs_close(&mvfs);
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_assets(argv[1],NULL,modelpath)==0);
 assert(cnk_test_gl.matrix_multiplications>0);
 assert(cnk_test_gl.invalid==0);
 /* The same production Vita entrypoint accepts THREE source-ID verified
  * LOD meshes for the slot, without implying any racer identity.
  * This is a test-only sidecar, never shipped as original game metadata. */
 assert(!fufs_open(argv[1],&mvfs));
 const unsigned lod_indices[3]={5,8,12};
 mf=fopen(modelpath,"wb");assert(mf);
 assert(fprintf(mf,"# HOST TEST ONLY. THREE ORIGINAL MODELS, UNVERIFIED CHARACTER\n"
  "0\t%u\t%08x\t%u\t%08x\t%u\t%08x\t5\t20\t"
  "1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n",
  lod_indices[0],mvfs.entries[lod_indices[0]].id,
  lod_indices[1],mvfs.entries[lod_indices[1]].id,
  lod_indices[2],mvfs.entries[lod_indices[2]].id)>0);
 assert(!fclose(mf));fufs_close(&mvfs);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_assets(argv[1],NULL,modelpath)==0);
 assert(cnk_test_gl.matrix_multiplications>0);
 assert(cnk_test_gl.invalid==0 && inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* An invalid model identity must fail BEFORE any GPU initialization. */
 FILE *invalid_file=fopen(modelpath,"wb");assert(invalid_file);
 assert(fprintf(invalid_file,"0\t0\t00000000\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n")>0);
 assert(!fclose(invalid_file));
 inits=shutdowns=0;
 assert(cnk_vita_native_run_with_assets(argv[1],NULL,modelpath)==-12);
 assert(!inits&&!shutdowns);
 assert(!remove(modelpath));
 /* A single ORIGINAL Zeebo WAV is temporarily paired with an app event.
  * This is a HOST TEST mapping, not a claim about the game's real cue. */
 char soundpath[]="/tmp/cnk_vita_sound_XXXXXX";
 int sfd=mkstemp(soundpath);assert(sfd>=0);
 FILE *sf=fdopen(sfd,"wb");assert(sf);
 assert(fprintf(sf,"# SYNTHETIC HOST TEST - NOT HISTORICAL SOUND IDENTITY\n"
    "APP\tRACE_STARTED\t44\t19df923d\t0.5\n")>0);
 assert(!fclose(sf));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 int opens=cnk_host_audio_opens,closes=cnk_host_audio_closes;
 int outputs=cnk_host_audio_outputs,signals=cnk_host_audio_signal_blocks;
extern int cnk_host_audio_fail_at;
 assert(cnk_vita_native_run_with_resources(argv[1],NULL,NULL,soundpath)==0);
 assert(cnk_host_audio_opens==opens+1&&cnk_host_audio_closes==closes+1);
 assert(cnk_host_audio_outputs>outputs);
 assert(cnk_host_audio_signal_blocks>signals);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* A mismatched ID must fail BEFORE starting the GPU or audio thread. */
 sf=fopen(soundpath,"wb");assert(sf);
 assert(fprintf(sf,"APP\tRACE_STARTED\t44\t00000000\t0.5\n")>0);
 assert(!fclose(sf));
 inits=shutdowns=0;
 opens=cnk_host_audio_opens;closes=cnk_host_audio_closes;
 assert(cnk_vita_native_run_with_resources(argv[1],NULL,NULL,soundpath)==-15);
 assert(!inits&&!shutdowns&&cnk_host_audio_opens==opens+1&&cnk_host_audio_closes==closes+1);
 /* Driver failure must be surfaced and the thread joined BEFORE cue free. */
 sf=fopen(soundpath,"wb");assert(sf);
 assert(fprintf(sf,"APP\tRACE_STARTED\t44\t19df923d\t0.5\n")>0);
 assert(!fclose(sf));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=audio_failure_playback;
 active_playback_len=sizeof(audio_failure_playback)/sizeof(audio_failure_playback[0]);
 cnk_host_audio_fail_at=cnk_host_audio_outputs+2;
 opens=cnk_host_audio_opens;closes=cnk_host_audio_closes;
 assert(cnk_vita_native_run_with_resources(argv[1],NULL,NULL,soundpath)==-20);
 cnk_host_audio_fail_at=0;
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 assert(cnk_host_audio_opens==opens+1&&cnk_host_audio_closes==closes+1);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(!remove(soundpath));
 /* SYNTHETIC soundtrack associations used ONLY inside this host test.
  * Track IDs are authentic Zeebo music, but menu/scene assignment is not. */
 char musicpath[]="/tmp/cnk_vita_music_XXXXXX";
 int music_fd=mkstemp(musicpath);assert(music_fd>=0);
 mf=fdopen(music_fd,"wb");assert(mf);
 assert(fprintf(mf,"# SYNTHETIC HOST TEST, NOT HISTORICAL MUSIC MAPPING\n"
   "SELECT\t*\t104\t0.8\t1\nRACE\t*\t122\t0.7\t1\n")>0);
 assert(!fclose(mf));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 opens=cnk_host_audio_opens;closes=cnk_host_audio_closes;
 outputs=cnk_host_audio_outputs;
 assert(cnk_vita_native_run_with_music(argv[1],NULL,NULL,NULL,musicpath,
   CNK_SOURCE_ROOT "/assets/music_wav")==0);
 assert(cnk_host_audio_opens==opens+1&&cnk_host_audio_closes==closes+1);
 assert(cnk_host_audio_outputs>outputs&&inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* A false original-MP3 ID must be rejected before graphics start. */
 mf=fopen(musicpath,"wb");assert(mf);
 assert(fprintf(mf,"RACE\t*\t999\t0.8\t1\n")>0);
 assert(!fclose(mf));
 inits=shutdowns=0;
 opens=cnk_host_audio_opens;closes=cnk_host_audio_closes;
 assert(cnk_vita_native_run_with_music(argv[1],NULL,NULL,NULL,musicpath,
   CNK_SOURCE_ROOT "/assets/music_wav")==-21);
 assert(inits==0&&shutdowns==0);
 assert(cnk_host_audio_opens==opens+1&&cnk_host_audio_closes==closes+1);
 assert(!remove(musicpath));
 /* Full host-recorded native app: source-model kart + source-verified
  * original Crash driver skin, with EXPLICIT TEST-ONLY slot/seat transforms.
  * This does not verify the real Zeebo driver's kart or seating offset. */
 char pairkart[]="/tmp/cnk_vita_pairkart_XXXXXX";
 int pk=mkstemp(pairkart);assert(pk>=0);
 FILE *kartfile=fdopen(pk,"wb");assert(kartfile);
 assert(!fufs_open(argv[1],&mvfs));
 assert(fprintf(kartfile,"# HOST SYNTHETIC: original model, unknown kart identity\n"
    "0\t5\t%08x\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n",
    mvfs.entries[5].id)>0);
 assert(!fclose(kartfile));fufs_close(&mvfs);
 char pairdriver[]="/tmp/cnk_vita_pairdriver_XXXXXX";
 int pd=mkstemp(pairdriver);assert(pd>=0);
 FILE *driverfile=fdopen(pd,"wb");assert(driverfile);
 assert(fprintf(driverfile,"# HOST SYNTHETIC: position AND kart-driver pairing not historical\n"
    "0\tcrash\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n")>0);
 assert(!fclose(driverfile));
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_driver_assets(argv[1],NULL,pairkart,
     NULL,NULL,NULL,pairdriver)==0);
 assert(cnk_test_gl.matrix_multiplications>=2);
 assert(cnk_test_gl.textured_calls>0);
 assert(cnk_test_gl.images_uploaded==3);
 assert(cnk_test_gl.handles_created==3&&cnk_test_gl.handles_deleted==3);
 assert(!cnk_test_gl.invalid&&inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* A driver sidecar without a source-checked kart is rejected BEFORE GPU. */
 inits=shutdowns=0;
 assert(cnk_vita_native_run_with_driver_assets(argv[1],NULL,NULL,
     NULL,NULL,NULL,pairdriver)==-25);
 assert(!inits&&!shutdowns);
 /* Atomic GPU failure rolls back all textures even after vglInit(). */
 controller_reads=swaps=vblank_waits=inits=shutdowns=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 cnk_test_gl.fail_upload_on_number=2;
 assert(cnk_vita_native_run_with_driver_assets(argv[1],NULL,pairkart,
     NULL,NULL,NULL,pairdriver)==-27);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 driverfile=fopen(pairdriver,"wb");assert(driverfile);
 assert(fprintf(driverfile,"0\tunknown\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n")>0);
 assert(!fclose(driverfile));
 inits=shutdowns=0;
 assert(cnk_vita_native_run_with_driver_assets(argv[1],NULL,pairkart,
     NULL,NULL,NULL,pairdriver)==-24);
 assert(!inits&&!shutdowns);
 /* New native app path: synthetic verified-format BODY PNG + authentic
  * source kart geometry plus original Crash driver. The PNG assignment in
  * this test is deliberately NOT a real historical kart identity. */
 driverfile=fopen(pairdriver,"wb");assert(driverfile);
 assert(fprintf(driverfile,"# SYNTHETIC kart-driver pair, test only\n"
    "0\tcrash\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n")>0);
 assert(!fclose(driverfile));
 char kartskinpath[]="/tmp/cnk_vita_kartskin_XXXXXX";
 int ks=mkstemp(kartskinpath);assert(ks>=0);
 FILE *skinfile=fdopen(ks,"wb");assert(skinfile);
 assert(fprintf(skinfile,"# SYNTHETIC PNG INDEX FOR TEST ONLY, not Zeebo map\n"
    "0\tNeoSprint01.png\t2\t01d12af6\n")>0);
 assert(!fclose(skinfile));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_kart_skins(argv[1],NULL,pairkart,
      NULL,NULL,NULL,pairdriver,kartskinpath)==0);
 assert(cnk_test_gl.images_uploaded==4); /* kart body + three characters */
 assert(cnk_test_gl.handles_created==4&&cnk_test_gl.handles_deleted==4);
 assert(cnk_test_gl.matrix_multiplications>=2 && cnk_test_gl.textured_calls>0);
 assert(!cnk_test_gl.invalid&&inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* A mismatched main-body material must fail, with no remaining GPU data. */
 skinfile=fopen(kartskinpath,"wb");assert(skinfile);
 assert(fprintf(skinfile,"0\tOtherUnverifiedBody.png\t2\t01d12af6\n")>0);
 assert(!fclose(skinfile));
 controller_reads=swaps=vblank_waits=inits=shutdowns=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_kart_skins(argv[1],NULL,pairkart,
      NULL,NULL,NULL,pairdriver,kartskinpath)==-29);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* Three genuine 3DOB source LODs with TWO test-only PNG associations.
  * The low-detail model contains TWO distinct POLY selectors for Wheel_t,
  * and the same wheel PNG is uploaded only once per racer. */
 kartfile=fopen(pairkart,"wb");assert(kartfile);
 assert(fprintf(kartfile,"# SYNTHETIC slot and switch distances, original 3DOB IDs\n"
    "0\t5\t02d15519\t8\t0404d06a\t12\t05384bbb\t5\t10\t"
    "1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\t0\t0\t0\t0\t1\n")>0);
 assert(!fclose(kartfile));
 skinfile=fopen(kartskinpath,"wb");assert(skinfile);
 assert(fprintf(skinfile,"# TEST ONLY, maps NOT historically verified\n"
    "0\tNeoSprint01.png\t2\t01d12af6\n"
    "0\tWheel_t.png\t93\t31abc114\n")>0);
 assert(!fclose(skinfile));
 controller_reads=swaps=vblank_waits=inits=shutdowns=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run_with_kart_skins(argv[1],NULL,pairkart,
      NULL,NULL,NULL,pairdriver,kartskinpath)==0);
 assert(cnk_test_gl.images_uploaded==5); /* kart body + wheel + 3 characters */
 assert(cnk_test_gl.handles_created==5&&cnk_test_gl.handles_deleted==5);
 assert(cnk_test_gl.textured_calls>0&&!cnk_test_gl.invalid);
 controller_reads=swaps=vblank_waits=inits=shutdowns=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 cnk_test_gl.fail_upload_on_number=2;
 assert(cnk_vita_native_run_with_kart_skins(argv[1],NULL,pairkart,
      NULL,NULL,NULL,pairdriver,kartskinpath)==-29);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(!remove(kartskinpath));
 assert(!remove(pairkart));assert(!remove(pairdriver));
 /* Opt-in source gallery: show three original textured characters on
  * SELECT without assigning any one of them to a kart. Host VitaGL stub. */
 assert(mkdir("app0:",0700)==0);
 FILE *gallery_flag=fopen("app0:/source_character_gallery.flag","wb");
 assert(gallery_flag);
 assert(fputs("Original-character debug gallery only\n",gallery_flag)>=0);
 assert(!fclose(gallery_flag));
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(cnk_test_gl.images_uploaded==3);
 assert(cnk_test_gl.handles_created==3&&cnk_test_gl.handles_deleted==3);
 assert(cnk_test_gl.textured_calls>=3);
 assert(!cnk_test_gl.invalid && !cnk_test_gl.matrix_stack_depth);
 assert(inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted && swaps==vblank_waits);
 /* Both opt-in viewers may be installed; source vehicle gallery takes
  * precedence and never uploads guessed body textures or driver identity. */
 FILE *vehicle_flag=fopen("app0:/source_kart_gallery.flag","wb");
 assert(vehicle_flag);
 assert(fputs("Original 3DOB vehicle geometry viewer only\n",vehicle_flag)>=0);
 assert(!fclose(vehicle_flag));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=playback;
 active_playback_len=sizeof(playback)/sizeof(playback[0]);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(cnk_test_gl.draw_calls>=6);
 assert(cnk_test_gl.images_uploaded==0 && cnk_test_gl.textured_calls==0);
 assert(cnk_test_gl.matrix_multiplications>=6);
 assert(cnk_test_gl.invalid==0 && cnk_test_gl.matrix_stack_depth==0);
 assert(inits==1&&shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted&&swaps==vblank_waits);
 /* R LOD navigation must affect actual GPU triangle counts, without
  * selecting a different race or creating a source body-texture mapping. */
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=kart_plain_playback;
 active_playback_len=sizeof(kart_plain_playback)/sizeof(kart_plain_playback[0]);
 assert(cnk_vita_native_run(argv[1])==0);
 uint64_t medium_triangles=cnk_test_gl.drawn_vertices;
 float baseline_camera[16];
 memcpy(baseline_camera,cnk_test_gl.last_camera_view,sizeof(baseline_camera));
 assert(medium_triangles>0 && !cnk_test_gl.invalid);
 /* L rotates the camera over the authentic, unchanged six source meshes;
  * the same amount of original geometry must be submitted to the GPU. */
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=gallery_orbit_playback;
 active_playback_len=sizeof(gallery_orbit_playback)/sizeof(gallery_orbit_playback[0]);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(cnk_test_gl.drawn_vertices==medium_triangles);
 assert(memcmp(baseline_camera,cnk_test_gl.last_camera_view,
        sizeof(baseline_camera))!=0);
 assert(!cnk_test_gl.invalid && !cnk_test_gl.matrix_stack_depth);

 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=kart_toggle_playback;
 active_playback_len=sizeof(kart_toggle_playback)/sizeof(kart_toggle_playback[0]);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(cnk_test_gl.drawn_vertices>0 && cnk_test_gl.drawn_vertices<medium_triangles);
 assert(cnk_test_gl.textured_calls==0 && !cnk_test_gl.invalid);
 /* The source gallery only accepts caller-reviewed optional PNG mappings.
  * Here the association is deliberately SYNTHETIC and deleted afterwards. */
 FILE *gallery_mapping=fopen("app0:/verified_gallery_textures.tsv","wb");
 assert(gallery_mapping);
 assert(fputs("# TEST ONLY, NOT VERIFIED ORIGINAL CAR BODY PNG\n"
              "0\tNeoSprint01.png\t2\t01d12af6\n",gallery_mapping)>=0);
 assert(!fclose(gallery_mapping));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 active_playback=kart_toggle_playback;
 active_playback_len=sizeof(kart_toggle_playback)/sizeof(kart_toggle_playback[0]);
 assert(cnk_vita_native_run(argv[1])==0);
 assert(cnk_test_gl.images_uploaded==1 && cnk_test_gl.textured_calls>0);
 assert(cnk_test_gl.handles_created==1 && cnk_test_gl.handles_deleted==1);
 assert(!cnk_test_gl.invalid && inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 /* A wrong original FUFS ID rejects the sidecar before drawing the gallery.
  * The application must still shut down GL and its other state. */
 gallery_mapping=fopen("app0:/verified_gallery_textures.tsv","wb");
 assert(gallery_mapping);
 assert(fputs("0\tNeoSprint01.png\t2\t01d12af7\n",gallery_mapping)>=0);
 assert(!fclose(gallery_mapping));
 controller_reads=swaps=vblank_waits=inits=shutdowns=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run(argv[1])==-42);
 assert(!cnk_test_gl.handles_created && !cnk_test_gl.textured_calls);
 assert(inits==1 && shutdowns==0 && cnk_test_gl.handles_created==cnk_test_gl.handles_deleted);
 assert(!remove("app0:/verified_gallery_textures.tsv"));
 assert(!remove("app0:/source_kart_gallery.flag"));
 assert(!remove("app0:/source_character_gallery.flag"));
 /* Advance 102: races without source-verified kart bindings now show
  * authentic 3DOB meshes as EXPLICITLY NON-CANONICAL visual placeholders.
  * The opt-out flag restores an empty vehicle view for exact comparisons. */
 active_playback=playback;active_playback_len=sizeof(playback)/sizeof(playback[0]);
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run(argv[1])==0);
 unsigned preview_models=cnk_test_gl.matrix_multiplications;
 uint64_t preview_calls=cnk_test_gl.draw_calls;
 assert(!cnk_test_gl.invalid);
 FILE *disable_flag=fopen("app0:/disable_dev_racer_fallback.flag","wb");
 assert(disable_flag);
 assert(fputs("TEST ONLY: suppress provisional models\n",disable_flag)>=0);
 assert(!fclose(disable_flag));
 controller_reads=swaps=vblank_waits=inits=shutdowns=overlay_quads=0;
 clocks=1000000;cnk_test_gl_reset(UINT32_MAX);
 assert(cnk_vita_native_run(argv[1])==0);
 fprintf(stderr,"advance 102 provisional kart draws %llu vs disabled %llu; matrices %u vs %u\n",
         (unsigned long long)preview_calls,(unsigned long long)cnk_test_gl.draw_calls,
         preview_models,cnk_test_gl.matrix_multiplications);
 assert(preview_calls>cnk_test_gl.draw_calls);
 assert(preview_models>cnk_test_gl.matrix_multiplications);
 assert(!cnk_test_gl.invalid&&cnk_test_gl.matrix_stack_depth==0);
 assert(!remove("app0:/disable_dev_racer_fallback.flag"));
 assert(!rmdir("app0:"));
 assert(remove(path)==0);cnk_png_catalog_free(&png);
 printf("PASS: native Vita game loop draws original track geometry with one optional SYNTHETIC texture binding, then releases GPU resources; IDs are not included in distribution.\n");
 puts("PASS: native PS Vita app wiring: source VFS, controls, race, camera, GPU calls, reviewed-cue audio output and safe shutdown (host-recorded only).");
 assert(save_directory_attempts>0);
 return 0;
}
