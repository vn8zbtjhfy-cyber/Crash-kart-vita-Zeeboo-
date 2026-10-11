#include "cnk_vita_audio_worker.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/audioout.h>
#include <string.h>

static int worker_entry(SceSize args,void *argp){
 cnk_vita_audio_worker *a=NULL;
 if(args!=sizeof(a)||!argp)return -1;
 memcpy(&a,argp,sizeof(a));
 if(!a)return -1;
 for(;;){
  int stop=0,rc=0;
  if(sceKernelLockMutex(a->mutex,1,NULL)<0)return -2;
  stop=a->stop_requested;
  /* Hold mutex only for mixing (never for blocking SceAudioOutOutput). */
  if(!stop){
   rc=cnk_audio_render(&a->output.mixer,a->output.buffer,CNK_VITA_AUDIO_FRAMES);
   if(rc<0)a->thread_error=-4;
  }
  if(sceKernelUnlockMutex(a->mutex,1)<0)return -3;
  if(stop)return 0;
  if(rc<0)return -4;
  rc=sceAudioOutOutput(a->output.port,a->output.buffer);
  if(rc<0){
   if(sceKernelLockMutex(a->mutex,1,NULL)<0)return -5;
   a->thread_error=rc;
   sceKernelUnlockMutex(a->mutex,1);
   return rc;
  }
 }
}
int cnk_vita_audio_worker_lock(cnk_vita_audio_worker *a){
 if(!a||a->mutex<0)return -1;
 return sceKernelLockMutex(a->mutex,1,NULL);
}
int cnk_vita_audio_worker_status(cnk_vita_audio_worker *a){
 if(!a||a->mutex<0)return -1;
 if(cnk_vita_audio_worker_lock(a)<0)return -2;
 int rc=a->thread_error;
 if(cnk_vita_audio_worker_unlock(a)<0)return -3;
 return rc;
}
int cnk_vita_audio_worker_unlock(cnk_vita_audio_worker *a){
 if(!a||a->mutex<0)return -1;
 return sceKernelUnlockMutex(a->mutex,1);
}
int cnk_vita_audio_worker_open(cnk_vita_audio_worker *a){
 if(!a)return -1;
 memset(a,0,sizeof(*a));a->mutex=-1;a->thread=-1;a->output.port=-1;
 if(cnk_vita_audio_open(&a->output))return -2;
 a->mutex=sceKernelCreateMutex("cnk_audio_mixer",0,0,NULL);
 if(a->mutex<0){cnk_vita_audio_close(&a->output);return -3;}
 return 0;
}
int cnk_vita_audio_worker_start(cnk_vita_audio_worker *a){
 if(!a||a->mutex<0||a->output.port<0||a->thread>=0)return -1;
 a->thread=sceKernelCreateThread("cnk_audio",worker_entry,0x10000100,0x10000,0,0,NULL);
 if(a->thread<0)return -2;
 cnk_vita_audio_worker *arg=a;
 if(sceKernelStartThread(a->thread,sizeof(arg),&arg)<0){
  sceKernelDeleteThread(a->thread);a->thread=-1;return -3;
 }
 a->running=1;
 return 0;
}
void cnk_vita_audio_worker_close(cnk_vita_audio_worker *a){
 if(!a)return;
 if(a->thread>=0){
  if(a->running){
   if(!cnk_vita_audio_worker_lock(a)){
    a->stop_requested=1;
    cnk_vita_audio_worker_unlock(a);
   }
   /* The thread does not hold the mutex while blocked on audio output. */
   sceKernelWaitThreadEnd(a->thread,NULL,NULL);
  }
  sceKernelDeleteThread(a->thread);a->thread=-1;
 }
 a->running=0;
 if(a->output.port>=0)cnk_vita_audio_close(&a->output);
 if(a->mutex>=0){sceKernelDeleteMutex(a->mutex);a->mutex=-1;}
}
