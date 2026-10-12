/* IMA ADPCM step/index reconstruction adapted from:
 * ZeebxTeam/zeebx-emu, src/audio/wav.rs (GPL-2.0-or-later).
 * The channel interleaving and nibble order follow Zeebx's reference.
 * The RIFF parser, bounded output, atomic failure and PCM16 output were
 * rewritten for the native C PS Vita audio mixer.
 * Upstream: https://github.com/ZeebxTeam/zeebx-emu
 * See third_party/zeebx/LICENSE and third_party/zeebx/NOTICE.md.
 */
#include "third_party/zeebx_ima.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p){
 return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
   ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void wr16(uint8_t *p,int16_t value){
 uint16_t u=(uint16_t)value;p[0]=(uint8_t)u;p[1]=(uint8_t)(u>>8);
}
static const int8_t index_table[16]={
 -1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8
};
static const int32_t step_table[89]={
 7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767
};
typedef struct {int32_t predictor,index;} ima_state;
static int16_t step_sample(ima_state *s,unsigned nibble){
 int32_t step=step_table[s->index];
 int32_t diff=step>>3;
 if(nibble&4u)diff+=step;
 if(nibble&2u)diff+=step>>1;
 if(nibble&1u)diff+=step>>2;
 s->predictor+=(nibble&8u)?-diff:diff;
 if(s->predictor>32767)s->predictor=32767;
 else if(s->predictor< -32768)s->predictor=-32768;
 s->index+=index_table[nibble&15u];
 if(s->index<0)s->index=0;
 if(s->index>88)s->index=88;
 return (int16_t)s->predictor;
}
int cnk_zeebx_ima_decode_wav(const uint8_t *wav,size_t length,
                             cnk_zeebx_ima_pcm *out){
 uint32_t rate=0,fact=0;uint16_t channels=0,align=0,bits=0,tag=0;
 size_t start=0,bytes=0;int have_fmt=0,have_data=0,have_fact=0;
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!wav||length<44||memcmp(wav,"RIFF",4)||
    memcmp(wav+8,"WAVE",4))return -2;
 const uint64_t end64=(uint64_t)rd32(wav+4)+8u;
 if(end64>length||end64<44)return -3;
 size_t end=(size_t)end64;
 for(size_t at=12;at<end;){
  if(end-at<8)return -4;
  uint32_t n=rd32(wav+at+4);
  if((size_t)n>end-at-8)return -5;
  const uint8_t *p=wav+at+8;
  if(!memcmp(wav+at,"fmt ",4)){
   if(have_fmt||n<16)return -6;
   tag=rd16(p);channels=rd16(p+2);
   rate=rd32(p+4);align=rd16(p+12);bits=rd16(p+14);
   have_fmt=1;
  }else if(!memcmp(wav+at,"data",4)){
   if(have_data)return -6;
   start=at+8;bytes=n;have_data=1;
  }else if(!memcmp(wav+at,"fact",4)){
   if(have_fact||n<4)return -6;
   fact=rd32(p);have_fact=1;
  }
  size_t skip=(size_t)n+8u+(n&1u);
  if(skip>end-at && (n&1u)&&skip==end-at+1u)skip--;
  if(skip>end-at)return -5;
  at+=skip;
 }
 if(!have_fmt||!have_data||!bytes||tag!=17u||
    (channels!=1u&&channels!=2u)||bits!=4u||
    rate<4000u||rate>192000u)return -7;
 size_t group=(size_t)channels*4u;
 if(align<group||align%group)return -8;
 /* Decode only complete channel interleave groups; the last block may be
  * shorter than block_align but must still contain complete group records. */
 size_t frames=0;
 for(size_t at=0;at<bytes;){
  size_t n=bytes-at<(size_t)align?bytes-at:(size_t)align;
  if(n<group||(n-group)%group)return -9;
  size_t chunk_frames=1u+(n-group)*2u/(size_t)channels;
  if(frames>UINT32_MAX-chunk_frames)return -10;
  frames+=chunk_frames;
  at+=n;
 }
 if(have_fact&&fact){
  if((size_t)fact>frames)return -11;
  frames=(size_t)fact;
 }
 if(!frames||frames>UINT32_MAX||
    frames>32u*1024u*1024u/((size_t)channels*2u))return -10;
 size_t pcm_bytes=frames*(size_t)channels*2u;
 uint8_t *samples=(uint8_t*)malloc(pcm_bytes);
 if(!samples)return -12;
 size_t written_frames=0;
 for(size_t at=0;at<bytes&&written_frames<frames;){
  size_t n=bytes-at<(size_t)align?bytes-at:(size_t)align;
  const uint8_t *block=wav+start+at;
  ima_state state[2]={{0,0},{0,0}};
  for(unsigned c=0;c<channels;c++){
   state[c].predictor=(int16_t)rd16(block+c*4u);
   state[c].index=block[c*4u+2u];
   if(state[c].index>88){
    free(samples);return -13;
   }
  }
  /* Each block begins with an uncompressed predictor per channel. */
  for(unsigned c=0;c<channels;c++)
   wr16(samples+(written_frames*channels+c)*2u,
        (int16_t)state[c].predictor);
  written_frames++;
  /* Stereo groups are four consecutive bytes PER channel. Decode into
   * temporary lanes and interleave sample-frames only after both lanes. */
  for(size_t g=group;g<n&&written_frames<frames;g+=group){
   int16_t decoded[2][8]={{0}};
   for(unsigned c=0;c<channels;c++)for(unsigned b=0;b<4;b++){
    uint8_t pair=block[g+c*4u+b];
    decoded[c][2u*b]=step_sample(&state[c],pair&15u);
    decoded[c][2u*b+1u]=step_sample(&state[c],pair>>4);
   }
   for(unsigned k=0;k<8&&written_frames<frames;k++,written_frames++)
    for(unsigned c=0;c<channels;c++)
     wr16(samples+(written_frames*channels+c)*2u,decoded[c][k]);
  }
  at+=n;
 }
 if(written_frames!=frames){
  free(samples);return -14;
 }
 *out=(cnk_zeebx_ima_pcm){samples,pcm_bytes,(uint32_t)frames,rate,channels};
 return 0;
}
