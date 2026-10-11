#include "cnk_vita_audio.h"
#include <psp2/audioout.h>
#include <string.h>
int cnk_vita_audio_open(cnk_vita_audio *a){
 if(!a)return -1;
 memset(a,0,sizeof(*a));a->port=-1;
 /* SceAudioOut MAIN specifically requires 48 kHz (not 44.1 kHz). */
 if(cnk_audio_init(&a->mixer,48000))return -2;
 int p=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                          (int)CNK_VITA_AUDIO_FRAMES,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(p<0)return p;
 a->port=p;return 0;
}
int cnk_vita_audio_pump(cnk_vita_audio *a){
 if(!a||a->port<0)return -1;
 if(cnk_audio_render(&a->mixer,a->buffer,CNK_VITA_AUDIO_FRAMES))return -2;
 return sceAudioOutOutput(a->port,a->buffer);
}
void cnk_vita_audio_close(cnk_vita_audio *a){
 if(!a)return;
 if(a->port>=0)sceAudioOutReleasePort(a->port);
 a->port=-1;
}
