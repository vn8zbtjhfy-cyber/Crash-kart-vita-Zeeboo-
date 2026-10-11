#include "cnk_sound_cues.h"
#include "cnk_race.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

typedef struct {const char *name;uint32_t bit;} cue_name;
static const cue_name app_names[]={
 {"TRACK_CHANGED",CNK_APP_EVENT_TRACK_CHANGED},
 {"RACE_STARTED",CNK_APP_EVENT_RACE_STARTED},
 {"RACE_RESTARTED",CNK_APP_EVENT_RACE_RESTARTED},
 {"PAUSED",CNK_APP_EVENT_PAUSED},
 {"RESUMED",CNK_APP_EVENT_RESUMED},
 {"BACK_TO_SELECT",CNK_APP_EVENT_BACK_TO_SELECT},
 {"RESULTS",CNK_APP_EVENT_RESULTS},
 {"COUNT_3",CNK_APP_EVENT_COUNT_3},
 {"COUNT_2",CNK_APP_EVENT_COUNT_2},
 {"COUNT_1",CNK_APP_EVENT_COUNT_1},
 {"GO",CNK_APP_EVENT_GO},
 {"DRIFT_START",CNK_APP_EVENT_DRIFT_START},
 {"DRIFT_STOP",CNK_APP_EVENT_DRIFT_STOP},
 {"BOOST_START",CNK_APP_EVENT_BOOST_START},
 {"LAUNCH_BOOST",CNK_APP_EVENT_LAUNCH_BOOST},
 {"BOOST_END",CNK_APP_EVENT_BOOST_END},
 {"ITEM_COLLECTED",CNK_APP_EVENT_ITEM_COLLECTED},
 {"WUMPA_COLLECTED",CNK_APP_EVENT_WUMPA_COLLECTED},
 {"ITEM_FIRED",CNK_APP_EVENT_ITEM_FIRED},
 {"ITEM_HIT",CNK_APP_EVENT_ITEM_HIT},
 {"MISSILE_FIRED",CNK_APP_EVENT_MISSILE_FIRED},
 {"HAZARD_DROPPED",CNK_APP_EVENT_HAZARD_DROPPED},
 {"MISSILE_HIT",CNK_APP_EVENT_MISSILE_HIT},
 {"TNT_HIT",CNK_APP_EVENT_TNT_HIT},
 {"OIL_HIT",CNK_APP_EVENT_OIL_HIT},
 {"WRONG_WAY_START",CNK_APP_EVENT_WRONG_WAY_START},
 {"WRONG_WAY_CLEAR",CNK_APP_EVENT_WRONG_WAY_CLEAR},
 {"KART_RECOVERED",CNK_APP_EVENT_KART_RECOVERED},
 {"NEW_RECORD",CNK_APP_EVENT_NEW_RECORD},
 {"MODE_CHANGED",CNK_APP_EVENT_MODE_CHANGED},
 {"LAPS_CHANGED",CNK_APP_EVENT_LAPS_CHANGED},
 {"INCOMING_PROJECTILE",CNK_APP_EVENT_INCOMING_PROJECTILE}
};
static const cue_name player_names[]={
 {"CHECKPOINT",CNK_RACE_GATE},{"LAP",CNK_RACE_LAP},
 {"FINISH",CNK_RACE_FINISH}
};
static uint32_t find_event(uint32_t domain,const char *name){
 const cue_name *list=(domain==CNK_SOUND_CUE_APP)?app_names:player_names;
 size_t n=(domain==CNK_SOUND_CUE_APP)?
   sizeof(app_names)/sizeof(app_names[0]):
   sizeof(player_names)/sizeof(player_names[0]);
 for(size_t i=0;i<n;i++)if(!strcmp(name,list[i].name))return list[i].bit;
 return 0;
}
static int parse_u32(const char *s,int base,uint32_t *result){
 char *end=NULL;unsigned long value;
 if(!s||!*s||isspace((unsigned char)*s)||*s=='-'||*s=='+')return -1;
 errno=0;value=strtoul(s,&end,base);
 if(errno||!end||*end||value>UINT32_MAX)return -1;
 *result=(uint32_t)value;return 0;
}
static int parse_volume(const char *s,float *result){
 char *end=NULL;float value;
 if(!s||!*s||isspace((unsigned char)*s))return -1;
 errno=0;value=strtof(s,&end);
 if(errno||!end||*end||!isfinite(value)||value<=0.f||value>1.f)return -1;
 *result=value;return 0;
}
void cnk_sound_cues_close(cnk_sound_cues *router){
 if(!router)return;
 /* A voice could still reference one of these buffers; stop it FIRST. */
 if(router->mixer){
  for(unsigned j=0;j<CNK_AUDIO_MAX_VOICES;j++){
   cnk_audio_voice *v=&router->mixer->voices[j];
   if(!v->active)continue;
   for(unsigned i=0;i<router->count;i++){
    if(router->cues[i].asset.wav.pcm==v->source.pcm &&
       router->cues[i].asset.wav.pcm){cnk_audio_stop(router->mixer,j);break;}
   }
  }
 }
 for(unsigned i=0;i<router->count;i++)cnk_wav_asset_free(&router->cues[i].asset);
 memset(router,0,sizeof(*router));
}
int cnk_sound_cues_read(cnk_sound_cues *router,cnk_audio_mixer *mixer,
                        const fufs_file *vfs,const char *path){
 FILE *f=NULL;char line[256];unsigned line_count=0;int result=-1;
 cnk_sound_cues pending={0};
 if(!router||!mixer||!mixer->sample_rate||!vfs||!vfs->entries||!path||!*path)return -1;
 f=fopen(path,"rb");if(!f)return -2;
 pending.mixer=mixer;
 while(fgets(line,sizeof(line),f)){
  char *parts[5];unsigned fields=0;
  size_t n=strlen(line);uint32_t index=0,id=0,domain=0,bit=0;float volume=0;
  line_count++;
  if(!n||line[n-1]!='\n'&&!feof(f)){result=-3;goto fail;}
  if(n&&line[n-1]=='\n')line[--n]=0;
  if(n&&line[n-1]=='\r')line[--n]=0;
  if(!n||line[0]=='#')continue;
  parts[fields++]=line;
  for(char *p=line;*p;p++)if(*p=='\t'){
   if(fields>=5){result=-4;goto fail;}
   *p=0;parts[fields++]=p+1;
  }
  if(fields!=5||pending.count>=CNK_SOUND_CUES_MAX){result=-4;goto fail;}
  if(!strcmp(parts[0],"APP"))domain=CNK_SOUND_CUE_APP;
  else if(!strcmp(parts[0],"PLAYER"))domain=CNK_SOUND_CUE_PLAYER;
  else {result=-5;goto fail;}
  bit=find_event(domain,parts[1]);
  if(!bit||parse_u32(parts[2],10,&index)||parse_u32(parts[3],16,&id)||
     parse_volume(parts[4],&volume)||index>=vfs->count||vfs->entries[index].id!=id){
   result=-6;goto fail;
  }
  for(unsigned j=0;j<pending.count;j++)if(pending.cues[j].domain==domain &&
     pending.cues[j].event_bit==bit){result=-7;goto fail;}
  cnk_sound_cue *cue=&pending.cues[pending.count];
  cue->domain=domain;cue->event_bit=bit;cue->vfs_index=index;
  cue->resource_id=id;cue->volume=volume;
  if(cnk_wav_asset_load(&cue->asset,vfs,index)){result=-8;goto fail;}
  pending.count++;
 }
 if(ferror(f)){result=-9;goto fail;}
 fclose(f);
 /* Optional but a present empty file should not hide an accidental typo. */
 if(pending.count==0){cnk_sound_cues_close(&pending);return -10;}
 cnk_sound_cues_close(router);
 *router=pending;
 return 0;
fail:
 fclose(f);
 cnk_sound_cues_close(&pending);
 (void)line_count;
 return result;
}
/* When all 16 mixer slots are occupied, reclaim only a non-looping
 * voice backed by THIS router's verified asset. Never evict music or another
 * audio system's voice. Prefer the most-completed effect for minimal loss. */
static int reclaim_oldest_sfx(cnk_sound_cues *router){
 int selected=-1;double completed=-1.0;
 for(unsigned j=0;j<CNK_AUDIO_MAX_VOICES;j++){
  cnk_audio_voice *voice=&router->mixer->voices[j];
  if(!voice->active||voice->loop||!voice->source.frame_count)continue;
  int owned=0;
  for(unsigned i=0;i<router->count;i++)
   if(router->cues[i].asset.wav.pcm &&
      voice->source.pcm==router->cues[i].asset.wav.pcm){owned=1;break;}
  if(!owned)continue;
  double progress=voice->frame_position/(double)voice->source.frame_count;
  if(progress>completed){completed=progress;selected=(int)j;}
 }
 if(selected<0)return 0;
 cnk_audio_stop(router->mixer,(unsigned)selected);
 return 1;
}
int cnk_sound_cues_dispatch(cnk_sound_cues *router,
                             const cnk_app_snapshot *snapshot,unsigned *played){
 if(!router||!router->mixer||!snapshot)return -1;
 unsigned count=0;
 for(unsigned i=0;i<router->count;i++){
  cnk_sound_cue *c=&router->cues[i];
  uint32_t events=c->domain==CNK_SOUND_CUE_APP?
                    snapshot->app_events:snapshot->race_events[0];
  if(!(events&c->event_bit))continue;
  int rc=cnk_audio_play(router->mixer,&c->asset.wav,c->volume,0);
  if(rc==-2){ /* Full polyphony is expected during crowded races. */
   if(reclaim_oldest_sfx(router))
    rc=cnk_audio_play(router->mixer,&c->asset.wav,c->volume,0);
   else continue; /* No owned effect can be reclaimed: drop SFX, not race. */
  }
  if(rc<0){if(played)*played=count;return -2;}
  count++;
 }
 if(played)*played=count;
 return 0;
}
