#include "cnk_music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

static const uint32_t ids[CNK_MUSIC_TRACKS]={104,122,277,292,316,331,393};
uint32_t cnk_music_original_index(unsigned track){
    return track<CNK_MUSIC_TRACKS?ids[track]:UINT32_MAX;
}
int cnk_music_init(cnk_music *p,cnk_audio_mixer *m){
    if(!p||!m||!m->sample_rate)return -1;
    memset(p,0,sizeof(*p));p->mixer=m;p->voice=-1;p->track=-1;return 0;
}
int cnk_music_stop(cnk_music *p){
    if(!p||!p->mixer)return -1;
    if(p->voice>=0){
        /* A finite song may have ended and its slot been reused by SFX.
         * Never silence that unrelated effect on a later music switch. */
        unsigned i=(unsigned)p->voice;
        int own=i<CNK_AUDIO_MAX_VOICES && p->mixer->voices[i].active &&
            p->mixer->voices[i].source.pcm==p->decoded.wav.pcm;
        int ret=own?cnk_audio_stop(p->mixer,i):0;
        p->voice=-1;
        if(ret)return ret;
    }
    return 0;
}
void cnk_music_close(cnk_music *p){
    if(!p)return;
    if(p->mixer)cnk_music_stop(p);
    cnk_wav_asset_free(&p->decoded);
    memset(p,0,sizeof(*p));p->voice=-1;p->track=-1;
}
int cnk_music_load(cnk_music *p,unsigned track,const char *path){
    FILE *f=NULL;long length;uint8_t *bytes=NULL;cnk_wav w;
    int result=-1;
    if(!p||!p->mixer||!path||track>=CNK_MUSIC_TRACKS)return -1;
    if(cnk_music_stop(p))return -2;
    cnk_wav_asset_free(&p->decoded);p->track=-1;
    f=fopen(path,"rb");if(!f)return -3;
    if(fseek(f,0,SEEK_END)!=0||(length=ftell(f))<44||
       length>32*1024*1024L||fseek(f,0,SEEK_SET)!=0){result=-4;goto fail;}
    bytes=(uint8_t *)malloc((size_t)length);
    if(!bytes){result=-5;goto fail;}
    if(fread(bytes,1,(size_t)length,f)!=(size_t)length){result=-6;goto fail;}
    if(cnk_wav_open(bytes,(size_t)length,&w)){result=-7;goto fail;}
    if(w.channels!=2||w.bits_per_sample!=16||w.sample_rate!=22050){
        result=-8;goto fail;
    }
    p->decoded.bytes=bytes;p->decoded.bytes_length=(size_t)length;
    p->decoded.wav=w;p->track=(int)track;
    fclose(f);return 0;
fail:
    free(bytes);fclose(f);return result;
}
int cnk_music_play(cnk_music *p,float volume,int loop){
    if(!p||!p->mixer||p->track<0||!p->decoded.bytes||
       !isfinite(volume)||volume<=0.0f||volume>1.0f)return -1;
    if(cnk_music_stop(p))return -2;
    int voice=cnk_audio_play(p->mixer,&p->decoded.wav,volume,loop);
    if(voice<0)return voice;
    p->voice=voice;return voice;
}
