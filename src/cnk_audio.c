#include "cnk_audio.h"
#include "third_party/zeebx_ima.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

static uint16_t rd16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}

int cnk_wav_open(const uint8_t *data,size_t size,cnk_wav *out){
 uint32_t riff_len,rate,byte_rate;uint16_t tag,channels,bits,block;
 int have_fmt=0,have_pcm=0;
 cnk_wav tmp={0};
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!data||size<44||memcmp(data,"RIFF",4)||memcmp(data+8,"WAVE",4))return -2;
 riff_len=rd32(data+4);
 if(riff_len<36||(uint64_t)riff_len+8>size)return -3;
 size_t end=(size_t)riff_len+8;
 for(size_t at=12;at<end;){
  if(end-at<8)return -4;
  uint32_t n=rd32(data+at+4);
  if((size_t)n>end-at-8)return -5;
  const uint8_t *p=data+at+8;
  if(!memcmp(data+at,"fmt ",4)){
   if(have_fmt||n<16)return -6;
   tag=rd16(p);channels=rd16(p+2);rate=rd32(p+4);
   byte_rate=rd32(p+8);block=rd16(p+12);bits=rd16(p+14);
   if(tag!=1||(channels!=1&&channels!=2)||(bits!=8&&bits!=16)||
      rate<4000||rate>192000||
      block!=channels*(bits/8)||
      (uint64_t)rate*(uint64_t)block!=byte_rate)return -7;
   tmp.sample_rate=rate;tmp.channels=channels;
   tmp.bits_per_sample=bits;tmp.frame_bytes=block;have_fmt=1;
  }else if(!memcmp(data+at,"data",4)){
   if(have_pcm)return -8;
   tmp.pcm=p;tmp.pcm_bytes=n;have_pcm=1;
  }
  size_t skip=(size_t)n+8+(n&1u);
  /* Some original Zeebo WAVs omit the optional pad byte on the final
   * odd-length chunk while preserving an exact RIFF payload length. */
  if(skip>end-at && (n&1u) && skip==end-at+1)skip--;
  if(skip>end-at)return -9;
  at+=skip;
 }
 if(!have_fmt||!have_pcm||!tmp.frame_bytes||!tmp.pcm_bytes||
    tmp.pcm_bytes%tmp.frame_bytes)return -10;
 if(tmp.pcm_bytes/tmp.frame_bytes>UINT32_MAX)return -11;
 tmp.frame_count=(uint32_t)(tmp.pcm_bytes/tmp.frame_bytes);
 *out=tmp;
 return 0;
}
/* A decoded sound is owned by the asset and remains valid while active
 * voices borrow its PCM pointer. Zero outputs atomically on failure. */
int cnk_wav_asset_open_memory(cnk_wav_asset *out,const uint8_t *data,size_t size){
 cnk_wav wav={0};
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!data||size<44||size>32u*1024u*1024u)return -2;
 if(!cnk_wav_open(data,size,&wav)){
  uint8_t *copy=(uint8_t*)malloc(size);
  if(!copy)return -3;
  memcpy(copy,data,size);
  /* wav.pcm is inside 'data'. Parse copied bytes to avoid dangling ptr. */
  if(cnk_wav_open(copy,size,&wav)){free(copy);return -4;}
  out->bytes=copy;out->bytes_length=size;out->wav=wav;
  return 0;
 }
 cnk_zeebx_ima_pcm decoded={0};
 if(cnk_zeebx_ima_decode_wav(data,size,&decoded))return -5;
 wav.pcm=decoded.samples;
 wav.pcm_bytes=decoded.pcm_bytes;
 wav.frame_count=decoded.frame_count;
 wav.sample_rate=decoded.sample_rate;
 wav.channels=decoded.channels;
 wav.bits_per_sample=16;
 wav.frame_bytes=(uint16_t)(decoded.channels*2u);
 out->bytes=decoded.samples;
 out->bytes_length=decoded.pcm_bytes;
 out->wav=wav;
 return 0;
}
int cnk_wav_asset_load(cnk_wav_asset *out,const fufs_file *vfs,uint32_t index){
 uint8_t *p=NULL;size_t n=0;
 if(!out)return -1;
 memset(out,0,sizeof(*out));
 if(!vfs)return -1;
 if(fufs_extract(vfs,index,&p,&n))return -2;
 int result=cnk_wav_asset_open_memory(out,p,n);
 free(p);
 return result?-3:0;
}
void cnk_wav_asset_free(cnk_wav_asset *asset){
 if(!asset)return;
 free(asset->bytes);memset(asset,0,sizeof(*asset));
}
int cnk_audio_init(cnk_audio_mixer *m,uint32_t sample_rate){
 if(!m||sample_rate<8000||sample_rate>192000)return -1;
 memset(m,0,sizeof(*m));m->sample_rate=sample_rate;m->master_volume=1.0f;
 return 0;
}
int cnk_audio_play(cnk_audio_mixer *m,const cnk_wav *src,float volume,int loop){
 if(!m||!m->sample_rate||!src||!src->pcm||!src->frame_count||
    !isfinite(volume)||volume<0.0f||volume>1.0f||
    (src->channels!=1&&src->channels!=2)||
    (src->bits_per_sample!=8&&src->bits_per_sample!=16)||
    src->frame_bytes!=src->channels*src->bits_per_sample/8||
    (size_t)src->frame_count>src->pcm_bytes/src->frame_bytes||
    src->sample_rate<4000||src->sample_rate>192000)return -1;
 for(unsigned i=0;i<CNK_AUDIO_MAX_VOICES;i++)if(!m->voices[i].active){
  cnk_audio_voice *v=&m->voices[i];
  memset(v,0,sizeof(*v));v->source=*src;
  v->volume=volume;v->loop=(uint8_t)!!loop;v->active=1;
  return (int)i;
 }
 return -2;
}
int cnk_audio_stop(cnk_audio_mixer *m,unsigned voice){
 if(!m||voice>=CNK_AUDIO_MAX_VOICES)return -1;
 memset(&m->voices[voice],0,sizeof(m->voices[voice]));return 0;
}
int cnk_audio_pause(cnk_audio_mixer *m,unsigned voice,int pause){
 if(!m||voice>=CNK_AUDIO_MAX_VOICES||!m->voices[voice].active)return -1;
 m->voices[voice].paused=(uint8_t)!!pause;return 0;
}
unsigned cnk_audio_active(const cnk_audio_mixer *m){
 unsigned n=0;if(!m)return 0;
 for(unsigned i=0;i<CNK_AUDIO_MAX_VOICES;i++)n+=!!m->voices[i].active;
 return n;
}
static float sample(const cnk_wav *s,uint32_t frame,uint32_t channel){
 const uint8_t *p=s->pcm+(size_t)frame*s->frame_bytes+
                        channel*(s->bits_per_sample/8);
 if(s->bits_per_sample==8)return (float)((int)p[0]-128)*256.0f;
 return (float)(int16_t)rd16(p);
}
static int16_t quantize(double v){
 /* Round only after summing ALL voices, then clip to avoid integer overflow. */
 if(v<=-32768.0)return INT16_MIN;
 if(v>=32767.0)return INT16_MAX;
 return (int16_t)(v>=0?v+0.5:v-0.5);
}
int cnk_audio_render(cnk_audio_mixer *m,int16_t *out,size_t frames){
 if(!m||!m->sample_rate||!out||frames>SIZE_MAX/2||
    !isfinite(m->master_volume)||m->master_volume<0.0f||
    m->master_volume>1.0f)return -1;
 for(size_t f=0;f<frames;f++){
  double left=0,right=0;
  for(unsigned k=0;k<CNK_AUDIO_MAX_VOICES;k++){
   cnk_audio_voice *v=&m->voices[k];
   if(!v->active||v->paused)continue;
   const cnk_wav *s=&v->source;
   if(v->frame_position>=s->frame_count){
    if(v->loop)v->frame_position=fmod(v->frame_position,(double)s->frame_count);
    else {v->active=0;continue;}
   }
   const uint32_t a=(uint32_t)v->frame_position;
   const uint32_t b=a+1<s->frame_count?a+1:(v->loop?0:a);
   const double frac=v->frame_position-(double)a;
   double l=sample(s,a,0)*(1-frac)+sample(s,b,0)*frac;
   double r=s->channels==2?(sample(s,a,1)*(1-frac)+sample(s,b,1)*frac):l;
   left+=l*v->volume;right+=r*v->volume;
   v->frame_position+=(double)s->sample_rate/(double)m->sample_rate;
   if(v->frame_position>=s->frame_count){
    if(v->loop)v->frame_position=fmod(v->frame_position,(double)s->frame_count);
    else v->active=0;
   }
  }
  out[2*f]=quantize(left*m->master_volume);
  out[2*f+1]=quantize(right*m->master_volume);
 }
 return 0;
}
