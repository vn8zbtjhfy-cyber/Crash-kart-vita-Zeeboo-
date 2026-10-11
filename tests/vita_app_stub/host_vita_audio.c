#define _POSIX_C_SOURCE 200809L
/* Host-only fake Sce kernel and audio output: exercises the production
 * audio thread, NOT evidence of Vita hardware playback. */
#include <psp2/kernel/threadmgr.h>
#include <psp2/audioout.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
static pthread_mutex_t mixer_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_t audio_thread;
static int thread_created,thread_started;
static SceKernelThreadEntry entry_fn;
static void *entry_arg;
static int audio_port=0;
int cnk_host_audio_opens,cnk_host_audio_closes,cnk_host_audio_outputs,cnk_host_audio_signal_blocks;
int cnk_host_audio_fail_at;
static void *thread_main(void*ignored){(void)ignored;
  int rc=entry_fn(sizeof(entry_arg),&entry_arg);
  return (void*)(intptr_t)rc;
}
SceUID sceKernelCreateThread(const char *name,SceKernelThreadEntry entry,int pri,SceSize stack,unsigned attr,int aff,const void *opt){
 (void)name;(void)pri;(void)stack;(void)attr;(void)aff;(void)opt;
 assert(!thread_created);entry_fn=entry;thread_created=1;return 15;
}
int sceKernelStartThread(SceUID id,SceSize len,void *arg){
 if(id!=15||!thread_created||len!=sizeof(void*))return -1;
 memcpy(&entry_arg,arg,sizeof(entry_arg));
 if(pthread_create(&audio_thread,NULL,thread_main,NULL))return -2;
 thread_started=1;return 0;
}
int sceKernelWaitThreadEnd(SceUID id,int *status,unsigned *timeout){
 (void)timeout;
 if(id!=15||!thread_started)return -1;
 void *ret=NULL;if(pthread_join(audio_thread,&ret))return -2;
 if(status)*status=(int)(intptr_t)ret;
 thread_started=0;return 0;
}
int sceKernelDeleteThread(SceUID id){if(id!=15||thread_started)return -1;thread_created=0;return 0;}
SceUID sceKernelCreateMutex(const char*name,unsigned attr,int initial,void*opt){(void)name;(void)attr;(void)initial;(void)opt;return 16;}
int sceKernelLockMutex(SceUID id,int count,unsigned*timeout){(void)timeout;return id==16&&count==1?pthread_mutex_lock(&mixer_lock):-1;}
int sceKernelUnlockMutex(SceUID id,int count){return id==16&&count==1?pthread_mutex_unlock(&mixer_lock):-1;}
int sceKernelDeleteMutex(SceUID id){return id==16?0:-1;}
int sceAudioOutOpenPort(int type,int len,int freq,int mode){
 assert(type==SCE_AUDIO_OUT_PORT_TYPE_MAIN&&len==512&&freq==48000&&mode==SCE_AUDIO_OUT_MODE_STEREO);
 assert(!audio_port);cnk_host_audio_opens++;audio_port=7;return audio_port;
}
int sceAudioOutOutput(int port,const void*buf){
 assert(port==7&&buf);
 const int16_t *samples=(const int16_t*)buf;
 for(int i=0;i<1024;i++)if(samples[i]){cnk_host_audio_signal_blocks++;break;}
 cnk_host_audio_outputs++;
 if(cnk_host_audio_fail_at>0 && cnk_host_audio_outputs>=cnk_host_audio_fail_at)return -123;
 struct timespec slice={0,1000000};nanosleep(&slice,NULL);
 return 0;
}
int sceAudioOutReleasePort(int port){assert(port==7&&audio_port==7);audio_port=0;cnk_host_audio_closes++;return 0;}
