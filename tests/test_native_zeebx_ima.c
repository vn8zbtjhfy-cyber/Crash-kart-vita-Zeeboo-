/* GPL-2.0-or-later codec regression using synthetic WAV bytes only.
 * The 0x44 -> [0,7,17] sample pattern is documented in the upstream
 * ZeebxTeam/zeebx-emu src/audio/wav.rs test suite. No game ROM required. */
#include "cnk_audio.h"
#include "third_party/zeebx_ima.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put16(uint8_t *p,uint16_t v){
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void put32(uint8_t *p,uint32_t v){
 for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));
}
static int16_t get16(const uint8_t *p){
 return (int16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);
}
/* Build a minimal RIFF fmt+fact?+data WAVE to exercise real asset wiring. */
static size_t wave(uint8_t *out,unsigned channels,unsigned fact_frames){
 memset(out,0,128);
 memcpy(out,"RIFF",4);memcpy(out+8,"WAVEfmt ",8);
 put32(out+16,16);
 put16(out+20,17);put16(out+22,(uint16_t)channels);
 put32(out+24,8000);put32(out+28,8000);
 put16(out+32,(uint16_t)(channels*8));put16(out+34,4);
 size_t at=36;
 if(fact_frames){
  memcpy(out+at,"fact",4);put32(out+at+4,4);
  put32(out+at+8,fact_frames);at+=12;
 }
 memcpy(out+at,"data",4);put32(out+at+4,(uint32_t)(channels*8));
 at+=8;
 /* Predictor 0 and step index 0 per channel, followed by 4-byte lane.
  * Low nibble decoded before high nibble; each 0x44 gives 7 then 17. */
 for(unsigned c=0;c<channels;c++){
  put16(out+at+c*4,(uint16_t)(c==0?0:100));
  out[at+c*4+2]=0;
 }
 for(unsigned c=0;c<channels;c++){
  out[at+4*channels+4*c]=0x44;
 }
 at+=channels*8;
 put32(out+4,(uint32_t)at-8u);
 return at;
}
int main(void){
 uint8_t wav[128];cnk_wav_asset a={0};
 assert(cnk_wav_asset_open_memory(NULL,wav,8)<0);
 size_t len=wave(wav,1,0);
 assert(!cnk_wav_asset_open_memory(&a,wav,len));
 assert(a.bytes && a.wav.pcm==a.bytes && a.wav.frame_count==9u);
 assert(a.wav.channels==1 && a.wav.sample_rate==8000u &&
        a.wav.bits_per_sample==16 && a.wav.frame_bytes==2);
 assert(get16(a.wav.pcm)==0);
 assert(get16(a.wav.pcm+2)==7);
 assert(get16(a.wav.pcm+4)==17);
 /* The converted bytes are independent of the original WAV storage. */
 memset(wav,0,len);
 assert(get16(a.wav.pcm+4)==17);
 cnk_audio_mixer mixer={0};int16_t rendered[6]={0};
 assert(!cnk_audio_init(&mixer,8000));
 assert(cnk_audio_play(&mixer,&a.wav,1.0f,0)>=0);
 assert(!cnk_audio_render(&mixer,rendered,3));
 assert(rendered[0]==0 && rendered[1]==0);
 assert(rendered[2]==7 && rendered[3]==7);
 assert(rendered[4]==17 && rendered[5]==17);
 cnk_audio_stop(&mixer,0);cnk_wav_asset_free(&a);
 assert(a.bytes==NULL && a.wav.pcm==NULL);

 len=wave(wav,2,0);
 assert(!cnk_wav_asset_open_memory(&a,wav,len));
 assert(a.wav.channels==2 && a.wav.frame_count==9u);
 assert(get16(a.wav.pcm)==0);
 assert(get16(a.wav.pcm+2)==100);
 assert(get16(a.wav.pcm+4)==7);
 assert(get16(a.wav.pcm+6)==107);
 assert(get16(a.wav.pcm+8)==17);
 assert(get16(a.wav.pcm+10)==117);
 cnk_wav_asset_free(&a);
 /* 'fact' trims padding in a block, without losing stereo alignment. */
 len=wave(wav,1,3);
 assert(!cnk_wav_asset_open_memory(&a,wav,len));
 assert(a.wav.frame_count==3 && a.wav.pcm_bytes==6);
 cnk_wav_asset_free(&a);
 /* Invalid predictor index fails cleanly and atomically. */
 len=wave(wav,1,0);
 wav[len-8+2]=89;
 assert(cnk_wav_asset_open_memory(&a,wav,len)<0);
 assert(a.bytes==NULL && a.wav.pcm==NULL);
 /* Reject malformed block alignment, bad RIFF and unexpected codecs. */
 len=wave(wav,1,0);put16(wav+32,6);
 assert(cnk_wav_asset_open_memory(&a,wav,len)<0);
 len=wave(wav,1,0);put16(wav+20,85);
 assert(cnk_wav_asset_open_memory(&a,wav,len)<0);
 len=wave(wav,1,0);put32(wav+4,UINT32_MAX);
 assert(cnk_wav_asset_open_memory(&a,wav,len)<0);
 /* Existing PCM must still be a zero-copy-format (owned byte copy) asset. */
 memset(wav,0,sizeof(wav));
 memcpy(wav,"RIFF",4);put32(wav+4,38);
 memcpy(wav+8,"WAVEfmt ",8);put32(wav+16,16);
 put16(wav+20,1);put16(wav+22,1);
 put32(wav+24,8000);put32(wav+28,16000);
 put16(wav+32,2);put16(wav+34,16);
 memcpy(wav+36,"data",4);put32(wav+40,2);
 put16(wav+44,12345);
 assert(!cnk_wav_asset_open_memory(&a,wav,46));
 assert(a.wav.bits_per_sample==16 && a.wav.frame_count==1);
 assert(get16(a.wav.pcm)==12345);
 put16(wav+44,0);
 assert(get16(a.wav.pcm)==12345);
 cnk_wav_asset_free(&a);
 puts("PASS: Zeebx-derived IMA ADPCM mono/stereo/fact and mixer, existing PCM unchanged");
 return 0;
}
