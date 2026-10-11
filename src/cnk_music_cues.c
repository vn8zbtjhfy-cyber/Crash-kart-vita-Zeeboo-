#include "cnk_music_cues.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

static int parse_decimal(const char *s,unsigned *number){
    unsigned long value;char *end;
    if(!s||!*s||!isdigit((unsigned char)*s))return -1;
    errno=0;value=strtoul(s,&end,10);
    if(errno||*end||value>UINT_MAX)return -1;
    *number=(unsigned)value;return 0;
}
static int state_from_text(const char *s,cnk_frontend_state *out){
    if(!strcmp(s,"SELECT"))*out=CNK_FRONTEND_SELECT;
    else if(!strcmp(s,"RACE"))*out=CNK_FRONTEND_RACE;
    else if(!strcmp(s,"PAUSE"))*out=CNK_FRONTEND_PAUSE;
    else if(!strcmp(s,"RESULTS"))*out=CNK_FRONTEND_RESULTS;
    else return -1;
    return 0;
}
static int parse_line(char *line,cnk_music_cue *out){
    char *fields[5],*p=line,*tab,*end;
    unsigned original=0,scene=0,lp=0;
    for(unsigned i=0;i<5;i++){
        fields[i]=p;
        if(i<4){tab=strchr(p,'\t');if(!tab)return -1;*tab='\0';p=tab+1;}
    }
    if(strchr(fields[4],'\t'))return -1;
    if(state_from_text(fields[0],&out->state))return -1;
    if(!strcmp(fields[1],"*"))out->scene_index=UINT32_MAX;
    else{
        if(out->state!=CNK_FRONTEND_RACE||parse_decimal(fields[1],&scene))return -1;
        out->scene_index=scene;
    }
    if(out->state!=CNK_FRONTEND_RACE && out->scene_index!=UINT32_MAX)return -1;
    if(parse_decimal(fields[2],&original))return -1;
    out->track=CNK_MUSIC_TRACKS;
    for(unsigned i=0;i<CNK_MUSIC_TRACKS;i++)
        if(cnk_music_original_index(i)==original){out->track=i;break;}
    if(out->track==CNK_MUSIC_TRACKS)return -1;
    errno=0;out->volume=strtof(fields[3],&end);
    if(errno||end==fields[3]||*end||!isfinite(out->volume)||
       out->volume<=0||out->volume>1)return -1;
    if(parse_decimal(fields[4],&lp)||lp>1)return -1;
    out->loop=(int)lp;
    return 0;
}
int cnk_music_cues_read(cnk_music_cues *cue,cnk_audio_mixer *mixer,
                        const char *tsv_path,const char *wav_directory){
    FILE *f;char line[256];cnk_music_cues temporary;
    if(!cue||cue->initialized||!mixer||!tsv_path||!wav_directory||!mixer->sample_rate||
       strlen(wav_directory)>=CNK_MUSIC_DIR_MAX-20)return -1;
    f=fopen(tsv_path,"rb");if(!f)return errno==ENOENT?-2:-3;
    memset(&temporary,0,sizeof(temporary));
    if(cnk_music_init(&temporary.player,mixer)){fclose(f);return -4;}
    memcpy(temporary.directory,wav_directory,strlen(wav_directory)+1);
    while(fgets(line,sizeof(line),f)){
        cnk_music_cue c;size_t len=strlen(line);
        if(len && line[len-1]!='\n'&&!feof(f)){fclose(f);return -5;}
        while(len && (line[len-1]=='\r'||line[len-1]=='\n'))line[--len]='\0';
        if(!len||line[0]=='#')continue;
        if(temporary.count==CNK_MUSIC_CUES_MAX||parse_line(line,&c)){
            fclose(f);return -6;
        }
        for(unsigned i=0;i<temporary.count;i++){
            const cnk_music_cue *a=&temporary.cues[i];
            if(a->state==c.state&&a->scene_index==c.scene_index){fclose(f);return -7;}
        }
        temporary.cues[temporary.count++]=c;
    }
    if(ferror(f)||!feof(f)){fclose(f);return -8;}
    fclose(f);
    if(!temporary.count)return -9;
    temporary.initialized=1;
    /* Only call when cue has no active music; startup loads maps before
     * creating the audio worker thread. */
    *cue=temporary;
    return 0;
}
static const cnk_music_cue *find_cue(const cnk_music_cues *r,const cnk_app_snapshot *s){
    const cnk_music_cue *wild=NULL;
    for(unsigned i=0;i<r->count;i++){
        const cnk_music_cue *c=&r->cues[i];
        if(c->state!=s->state)continue;
        if(c->scene_index==s->selected_vfs_index)return c;
        if(c->scene_index==UINT32_MAX)wild=c;
    }
    return wild;
}
/* Never pause a reused mixer slot that no longer belongs to our track. */
static int owned_music_voice(const cnk_music *p){
 int v=p->voice;
 return v>=0&&v<(int)CNK_AUDIO_MAX_VOICES &&
        p->mixer->voices[v].active &&
        p->mixer->voices[v].source.pcm==p->decoded.wav.pcm;
}
static int set_music_paused(cnk_music_cues *r,int pause,
                           cnk_music_mutex_fn lock,cnk_music_mutex_fn unlock,
                           void *context){
 if(lock(context)<0)return -1;
 if(owned_music_voice(&r->player))
  cnk_audio_pause(r->player.mixer,(unsigned)r->player.voice,pause);
 r->race_music_paused=!!pause;
 if(unlock(context)<0)return -2;
 return 0;
}
int cnk_music_cues_dispatch(cnk_music_cues *r,const cnk_app_snapshot *snapshot,
                            cnk_music_mutex_fn lock,cnk_music_mutex_fn unlock,
                            void *context,unsigned *changed){
    const cnk_music_cue *wanted;
    cnk_music prepared;
    cnk_audio_mixer *mixer;
    char path[CNK_MUSIC_DIR_MAX+24];
    int needs_change,result=0;
    if(changed)*changed=0;
    if(!r||!r->initialized||!snapshot||!lock||!unlock)return -1;
    wanted=find_cue(r,snapshot);
    /* No soundtrack assignment for PAUSE: freeze the existing RACE PCM
     * cursor. The previous implementation unloaded it and restarted the
     * track from the beginning on every unpause. Explicit PAUSE cues win. */
    if(snapshot->state==CNK_FRONTEND_PAUSE&&!wanted){
        if(!r->race_music_paused){
            if(set_music_paused(r,1,lock,unlock,context))return -9;
            if(changed)*changed=1;
        }
        return 0;
    }
    if(r->race_music_paused){
        if(set_music_paused(r,0,lock,unlock,context))return -10;
        if(changed)*changed=1;
    }
    /* Reusing the same track preserves time position on mode transitions.
     * The app uses a single explicitly identified loop policy. */
    needs_change=(!wanted&&r->player.track>=0)||
                 (wanted&&(r->player.track!=(int)wanted->track||r->player.voice<0||
                 r->playing_volume!=wanted->volume||r->playing_loop!=wanted->loop));
    if(!needs_change)return 0;
    memset(&prepared,0,sizeof(prepared));
    if(wanted){
        if(cnk_music_init(&prepared,r->player.mixer))return -2;
        int n=snprintf(path,sizeof(path),"%s/%u.wav",r->directory,
                       cnk_music_original_index(wanted->track));
        if(n<=0||(size_t)n>=sizeof(path))return -3;
        result=cnk_music_load(&prepared,wanted->track,path);
        if(result){cnk_music_close(&prepared);return -4;}
    }
    if(lock(context)<0){if(wanted)cnk_music_close(&prepared);return -5;}
    mixer=r->player.mixer;
    /* Voice update and release of old PCM happen entirely under the same
     * lock as the Vita audio mixing thread. */
    cnk_music_close(&r->player);
    if(wanted){
        r->player=prepared;
        if(cnk_music_play(&r->player,wanted->volume,wanted->loop)<0){
            cnk_music_close(&r->player);result=-6;
        }else{
            r->playing_volume=wanted->volume;
            r->playing_loop=wanted->loop;
        }
    }else{
        r->playing_volume=0.0f;
        r->playing_loop=0;
        if(cnk_music_init(&r->player,mixer)){
            result=-7;
        }
    }
    if(unlock(context)<0)return -8;
    if(changed)*changed=1;
    return result;
}
void cnk_music_cues_close(cnk_music_cues *cue){
    if(!cue)return;
    if(cue->initialized)cnk_music_close(&cue->player);
    memset(cue,0,sizeof(*cue));
}
