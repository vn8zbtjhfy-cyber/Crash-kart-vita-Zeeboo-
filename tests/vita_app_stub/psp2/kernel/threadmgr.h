#ifndef CNK_HOST_THREADMGR_H
#define CNK_HOST_THREADMGR_H
#include <psp2/types.h>
typedef int (*SceKernelThreadEntry)(SceSize,void*);
SceUID sceKernelCreateThread(const char*,SceKernelThreadEntry,int,SceSize,unsigned,int,const void*);
int sceKernelStartThread(SceUID,SceSize,void*);
int sceKernelWaitThreadEnd(SceUID,int*,unsigned*);
int sceKernelDeleteThread(SceUID);
SceUID sceKernelCreateMutex(const char*,unsigned,int,void*);
int sceKernelLockMutex(SceUID,int,unsigned*);
int sceKernelUnlockMutex(SceUID,int);
int sceKernelDeleteMutex(SceUID);
#endif
