#ifndef CNK_TEST_PROCESSMGR_H
#define CNK_TEST_PROCESSMGR_H
#include <stdint.h>
uint64_t sceKernelGetProcessTimeWide(void);
void sceKernelExitProcess(int status);
#endif
