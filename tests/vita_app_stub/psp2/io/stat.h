#ifndef CNK_HOST_VITA_STAT_STUB_H
#define CNK_HOST_VITA_STAT_STUB_H
/* Host-only function signature. Real build includes VitaSDK's stat.h. */
int sceIoMkdir(const char *dir,unsigned int mode);
#endif
