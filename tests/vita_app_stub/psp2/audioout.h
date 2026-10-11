#ifndef CNK_HOST_AUDIOOUT_H
#define CNK_HOST_AUDIOOUT_H
#define SCE_AUDIO_OUT_PORT_TYPE_MAIN 0
#define SCE_AUDIO_OUT_MODE_STEREO 1
int sceAudioOutOpenPort(int,int,int,int);
int sceAudioOutOutput(int,const void*);
int sceAudioOutReleasePort(int);
#endif
