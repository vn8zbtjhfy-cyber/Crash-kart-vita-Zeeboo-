#include "cnk_audio.h"
#include "fufs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Valid synthetic RIFF PCM, for deterministic checks of interpolation. */
static const uint8_t tiny[] = {
 'R','I','F','F',40,0,0,0,'W','A','V','E',
 'f','m','t',' ',16,0,0,0,1,0,1,0,0x40,0x1f,0,0,0x40,0x1f,0,0,1,0,8,0,
 'd','a','t','a',3,0,0,0,128,192,255,0
};
static int check_samples(void){
 cnk_wav src;cnk_audio_mixer m;int16_t dst[20]={0};
 assert(cnk_wav_open(tiny,sizeof(tiny),&src)==0);
 assert(src.channels==1&&src.frame_count==3&&src.sample_rate==8000);
 assert(cnk_audio_init(&m,16000)==0);
 int channel=cnk_audio_play(&m,&src,1,0);
 assert(channel==0);
 assert(cnk_audio_render(&m,dst,6)==0);
 assert(dst[0]==0&&dst[1]==0);
 assert(dst[2]==8192&&dst[3]==8192);
 assert(dst[4]==16384&&dst[5]==16384);
 assert(dst[6]==24448&&dst[7]==24448);
 assert(dst[8]==32512&&dst[9]==32512);
 assert(dst[10]==32512&&dst[11]==32512);
 assert(cnk_audio_active(&m)==0);
 assert(cnk_audio_render(&m,dst,4)==0);
 for(unsigned i=0;i<8;i++)assert(dst[i]==0);
 assert(cnk_audio_play(&m,&src,1,1)==0);
 assert(cnk_audio_render(&m,dst,10)==0);
 assert(cnk_audio_active(&m)==1);
 assert(dst[0]==0&&dst[6]==24448);
 assert(cnk_audio_stop(&m,0)==0&&cnk_audio_active(&m)==0);
 assert(cnk_audio_play(&m,&src,1,1)==0);
 assert(!cnk_audio_pause(&m,0,1));
 double paused_position=m.voices[0].frame_position;
 assert(!cnk_audio_render(&m,dst,6));
 for(unsigned i=0;i<12;i++)assert(dst[i]==0);
 assert(m.voices[0].frame_position==paused_position);
 assert(!cnk_audio_pause(&m,0,0));
 assert(!cnk_audio_render(&m,dst,4));
 assert(m.voices[0].frame_position>paused_position);
 assert(!cnk_audio_stop(&m,0));
 assert(cnk_audio_pause(&m,0,1)<0);
 assert(cnk_audio_pause(&m,CNK_AUDIO_MAX_VOICES,1)<0);
 assert(cnk_audio_play(&m,&src,1,0)==0);
 assert(cnk_audio_play(&m,&src,1,0)==1);
 assert(cnk_audio_render(&m,dst,6)==0);
 assert(dst[2]==16384); /* Two voices mixed, not overwritten. */
 uint8_t invalid[sizeof(tiny)];memcpy(invalid,tiny,sizeof(tiny));
 invalid[20]=3;assert(cnk_wav_open(invalid,sizeof(invalid),&src)<0);
 memcpy(invalid,tiny,sizeof(tiny));invalid[4]=0xff;
 assert(cnk_wav_open(invalid,sizeof(invalid),&src)<0);
 assert(cnk_wav_open(tiny,sizeof(tiny)-1,&src)<0);
 assert(cnk_audio_init(&m,0)<0);
 assert(cnk_audio_render(NULL,dst,1)<0);
 return 0;
}
int main(int argc,char **argv){
 assert(argc==2);
 check_samples();
 fufs_file f={0};assert(fufs_open(argv[1],&f)==0);
 uint32_t wav_count=0,mp3_count=0;
 uint32_t total_samples=0;
 cnk_audio_mixer mixer;assert(cnk_audio_init(&mixer,44100)==0);
 for(uint32_t i=0;i<f.count;i++){
  unsigned char *p=NULL;size_t n=0;
  assert(fufs_extract(&f,i,&p,&n)==0);
  const char *type=fufs_type(p,n);
  if(strcmp(type,"wav")==0){
   cnk_wav wav;
   int parse=cnk_wav_open(p,n,&wav); if(parse)fprintf(stderr,"wav index %u size=%zu parse=%d riff=%u\n",i,n,parse,(unsigned)(p[4]|p[5]<<8|p[6]<<16|p[7]<<24)); assert(parse==0);
   assert(wav.frame_count>0&&wav.sample_rate>=6000);
   assert((size_t)wav.frame_count*wav.frame_bytes==wav.pcm_bytes);
   int channel=cnk_audio_play(&mixer,&wav,0.5f,0);
   assert(channel>=0);
   int16_t output[2*512];
   assert(cnk_audio_render(&mixer,output,512)==0);
   assert(cnk_audio_stop(&mixer,(unsigned)channel)==0);
   cnk_wav_asset owned;
   assert(cnk_wav_asset_load(&owned,&f,i)==0);
   assert(owned.wav.pcm_bytes==wav.pcm_bytes);
   assert(memcmp(owned.wav.pcm,wav.pcm,wav.pcm_bytes)==0);
   cnk_wav_asset_free(&owned);
   assert(owned.bytes==NULL);
   wav_count++;total_samples+=wav.frame_count;
  }else if(strcmp(type,"mp3")==0){mp3_count++;}
  free(p);
 }
 assert(wav_count==29&&mp3_count==7);
 assert(cnk_audio_active(&mixer)==0);
 printf("29 WAV originales PCM validados, 7 MP3 catalogados, muestras=%u\n",total_samples);
 fufs_close(&f);
 return 0;
}
