#include "cnk_music.h"
#include "cnk_audio.h"
#include "fufs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

static const unsigned originals[7]={104,122,277,292,316,331,393};
int main(int argc,char **argv){
    assert(argc==2);
    fufs_file rom={0};assert(fufs_open(argv[1],&rom)==0);
    cnk_audio_mixer m;assert(cnk_audio_init(&m,48000)==0);
    cnk_music p;assert(cnk_music_init(&p,&m)==0);
    assert(cnk_music_original_index(7)==UINT32_MAX);
    assert(cnk_music_load(&p,7,"nowhere")<0);
    assert(cnk_music_play(&p,0.7f,1)<0);
    assert(cnk_music_load(&p,0,"not_existing.mp3")<0);
    int16_t pcm[2*512];
    double total_length=0;
    for(unsigned n=0;n<7;n++){
        unsigned id=cnk_music_original_index(n);
        assert(id==originals[n]);
        char path[160];snprintf(path,sizeof(path),"%s/assets/music_wav/%03u.wav",CNK_SOURCE_ROOT,id);
        struct stat st;assert(stat(path,&st)==0&&st.st_size>50000);
        assert(cnk_music_load(&p,n,path)==0);
        assert(p.track==(int)n&&p.voice==-1);
        assert(p.decoded.wav.channels==2&&p.decoded.wav.bits_per_sample==16);
        assert(p.decoded.wav.sample_rate==22050);
        double duration=(double)p.decoded.wav.frame_count/22050.0;
        assert(duration>3&&duration<120);total_length+=duration;
        int handle=cnk_music_play(&p,0.3f,1);
        assert(handle>=0&&handle<(int)CNK_AUDIO_MAX_VOICES);
        assert(cnk_audio_active(&m)==1);
        double energy=0;
        for(unsigned k=0;k<100;k++){
            assert(cnk_audio_render(&m,pcm,512)==0);
            for(size_t s=0;s<1024;s++)energy+=abs((int)pcm[s]);
        }
        assert(energy>100000.0); /* A real audible signal, not silence. */
        /* SFX and music must mix without music overwriting sound effects. */
        cnk_wav_asset effect;
        assert(cnk_wav_asset_load(&effect,&rom,44)==0);
        int fx=cnk_audio_play(&m,&effect.wav,0.8f,0);
        assert(fx>=0&&fx!=handle);
        assert(cnk_audio_active(&m)==2);
        assert(cnk_audio_render(&m,pcm,512)==0);
        assert(cnk_audio_stop(&m,(unsigned)fx)==0);
        cnk_wav_asset_free(&effect);
        assert(cnk_music_play(&p,1.01f,0)<0);
        assert(cnk_music_play(&p,NAN,0)<0);
        assert(cnk_music_stop(&p)==0&&cnk_audio_active(&m)==0);
        assert(cnk_music_stop(&p)==0); /* idempotent */
        printf("track original %u: %.2fs, %u PCM frames OK\n",id,duration,p.decoded.wav.frame_count);
    }
    assert(total_length>230.0&&total_length<250.0);
    /* Regression: music ended naturally -> slot reused by effect -> stop
     * must NOT stop the effect that borrowed that now-free voice slot. */
    assert(cnk_music_play(&p,1,0)>=0);
    for(unsigned i=0;i<CNK_AUDIO_MAX_VOICES;i++){
        if(i!=(unsigned)p.voice)assert(!m.voices[i].active);
    }
    /* Position at last sample and render to force natural stop. */
    unsigned music_voice=(unsigned)p.voice;
    m.voices[music_voice].frame_position=p.decoded.wav.frame_count-0.1;
    assert(cnk_audio_render(&m,pcm,512)==0);
    assert(!m.voices[music_voice].active);
    cnk_wav_asset effect;
    assert(cnk_wav_asset_load(&effect,&rom,44)==0);
    int fx=cnk_audio_play(&m,&effect.wav,1,1);
    assert(fx==(int)music_voice);
    assert(cnk_music_stop(&p)==0);
    assert(m.voices[fx].active);
    assert(cnk_audio_stop(&m,(unsigned)fx)==0);
    cnk_wav_asset_free(&effect);
    cnk_music_close(&p);
    assert(p.decoded.bytes==NULL&&p.track==-1);
    fufs_close(&rom);
    puts("7 MP3 originales convertidos, mezclados y validados; seguridad de voces OK");
    return 0;
}
